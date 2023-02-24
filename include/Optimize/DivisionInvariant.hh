#pragma once

#include "Pass.hh"
#include "logging.hpp"

#include <cmath>

// 除常数优化
// 1. 处理除以 2 的倍数
// 2. 将取模改为减法和乘除操作
class DivisionInvariant : public Pass {
    using llu = unsigned long long;

  public:
    DivisionInvariant(Module *m) : Pass(m) {}
    int mylog2(int d) {
        int ret = 0;
        if (__builtin_popcount(d) > 1)
            ret = 1;
        while (d >>= 1) {
            ret++;
        }
        return ret;
    }

    void choose_multiplier(unsigned d, int prec, llu &mhigh, int &shpost, int &l) {
        l = mylog2(d);
        shpost = l;
        llu mlow = (1ull << (N + l)) / d;
        mhigh = ((1ull << (N + l)) + (1ull << (N + l - prec))) / d;
        LOG_INFO << d << " " << prec << " " << mhigh << " " << shpost << " " << l << " " << mlow;
        while ((mlow / 2 < mhigh / 2) and shpost > 0) {
            mlow /= 2;
            mhigh /= 2;
            shpost -= 1;
            LOG_INFO << d << " " << prec << " " << mhigh << " " << shpost << " " << l << " " << mlow;
        }
        // LOG_INFO << l << " " << (1ll << (l - 1)) << " " << d << " " << (1ull << l);

        assert(1 <= d);
        assert(1 <= prec and prec <= N);
        assert((l == 0 or (1ull << (l - 1)) < d) and d <= (1ull << l));
        assert(0 <= shpost and shpost <= l);
        assert(shpost == 0 or N + shpost <= l + prec);
        assert((1ull << (N + shpost)) < mhigh * d and
               mhigh * d <= (1ull << (N + shpost)) + ((1ull << (N + shpost)) >> prec));
    }

    void run() {
        auto func_list = m_->get_functions();
        for (auto func : func_list) {
            if (func->get_num_basic_blocks() == 0)
                continue;
            for (auto bb : func->get_basic_blocks()) {
                for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end();) {
                    if ((*it)->is_div() and is_constant_int((*it)->get_operand(1).get())) {
                        divide_by_invariant(it);
                    } else if ((*it)->is_srem()) {
                        replace_srem(it);
                        // ++it;
                    } else
                        ++it;
                }
            }
        }

        LOG_INFO << "division invariant pass processed " << ins_count << " instructions";
    }

    bool is_constant_int(Value *val) {
        if (std::dynamic_pointer_cast<ConstantInt>(val->shared_from_this()) != nullptr)
            return true;
        return false;
        // return typeid(*val) == typeid(*ConstantInt::get(0, m_));
    }

    void replace_srem(Instruction::self_iterator &it) {
        auto inst = *it;
        auto lhs = (*it)->get_operand(0)->shared_from_this();
        auto bb = std::dynamic_pointer_cast<BasicBlock>(inst->get_parent()->shared_from_this());

        if (auto rhs = std::dynamic_pointer_cast<ConstantInt>(inst->get_operand(1)->shared_from_this())) {
            int rhs_val = rhs->get_value();
            if (rhs_val == 0) {
                ++it;
                return;
            }
            if (std::abs(rhs_val) == 1) {
                inst->replace_all_use_with(ConstantInt::get(0, m_));
                ++it;
                return;
            }
            // if (__builtin_popcount(rhs_val) == 1) {
            //     // TODO 有 and 指令时可以优化

            int l, shpost, q;
            llu m;
            choose_multiplier(std::abs(rhs_val), N - 1, m, shpost, l);

            if (std::abs(rhs_val) == (1ull << l)) {
                //
                auto newval1 = BinaryInst::create(Type::get_int32_type(m_),
                                                  Instruction::ashr,
                                                  lhs,
                                                  ConstantInt::get(static_cast<int>(N - 1), m_),
                                                  nullptr);
                auto newval2 = BinaryInst::create(Type::get_int32_type(m_),
                                                  Instruction::lshr,
                                                  newval1,
                                                  ConstantInt::get(static_cast<int>(N - l), m_),
                                                  nullptr);
                auto newval3 = BinaryInst::create(Type::get_int32_type(m_), Instruction::add, lhs, newval2, nullptr);
                auto newval4 = BinaryInst::create(Type::get_int32_type(m_),
                                                  Instruction::and_,
                                                  newval3,
                                                  ConstantInt::get(~(std::abs(rhs_val) - 1), m_),
                                                  nullptr);
                auto newval5 = BinaryInst::create(Type::get_int32_type(m_), Instruction::sub, lhs, newval4, nullptr);

                newval1->set_parent(bb.get());
                newval2->set_parent(bb.get());
                newval3->set_parent(bb.get());
                newval4->set_parent(bb.get());
                newval5->set_parent(bb.get());

                auto val = *it;
                (*it)->remove_use_of_ops();
                it = bb->get_instructions().erase(it);

                bb->get_instructions().insert(it, newval1);
                bb->get_instructions().insert(it, newval2);
                bb->get_instructions().insert(it, newval3);
                bb->get_instructions().insert(it, newval4);
                bb->get_instructions().insert(it, newval5);
                // if (d < 0) {
                //     auto newval5 = BinaryInst::create(
                //         Type::get_int32_type(m_), Instruction::sub, ConstantInt::get(0, m_), newval4, nullptr);
                //     newval5->set_parent(bb.get());
                //     bb->get_instructions().insert(it, newval5);
                //     val->replace_all_use_with(newval5);
                // } else
                val->replace_all_use_with(newval5);

                // for (auto inst : bb->get_instructions())
                //     LOG
            }

            // }
        }

        ++ins_count;

        // 在后端使用 sdiv, mls 的指令
        // auto lhs = inst->get_operand(0)->shared_from_this();
        // auto rhs = inst->get_operand(1)->shared_from_this();
        // auto div_val = BinaryInst::create(Type::get_int32_type(m_), Instruction::sdiv, lhs, rhs, nullptr);
        // auto mul_val = BinaryInst::create(Type::get_int32_type(m_), Instruction::mul, div_val, rhs, nullptr);
        // auto minus_val = BinaryInst::create(Type::get_int32_type(m_), Instruction::sub, lhs, mul_val, nullptr);

        // div_val->set_parent(bb.get());
        // mul_val->set_parent(bb.get());
        // minus_val->set_parent(bb.get());

        // // 清理工作丢给死代码消除了
        // inst->replace_with(div_val, mul_val, minus_val);
        ++it;
    }

    void divide_by_invariant(Instruction::self_iterator &it) {
        ++ins_count;
        ConstantInt *constant = static_cast<ConstantInt *>((*it)->get_operand(1).get());
        auto d = ConstantInt::get_value(constant);
        // std::shared_ptr<Value> lhs{(*it)->get_operand(0).get(),
        //    [](Value *) {}};  // 阻止智能指针的自动释放
        auto bb = std::dynamic_pointer_cast<BasicBlock>((*it)->get_parent()->shared_from_this());
        auto lhs = (*it)->get_operand(0).get()->shared_from_this();

        if (d == 0) {
            ++it;
            return;
        }
        int l, shpost, q;
        llu m;
        choose_multiplier(std::abs(d), N - 1, m, shpost, l);
        LOG_INFO << d << " " << m << " " << shpost << " " << l;

        if (std::abs(d) == 1) {
            if (d == 1) {
                (*it)->replace_all_use_with(lhs);
                (*it)->remove_use_of_ops();
                it = bb->get_instructions().erase(it);
            } else {
                auto newval = BinaryInst::create(
                    Type::get_int32_type(m_), Instruction::sub, ConstantInt::get(0, m_), lhs, nullptr);
                newval->set_parent(bb.get());
                (*it)->replace_all_use_with(newval);
                (*it)->remove_use_of_ops();
                it = bb->get_instructions().erase(it);
                bb->get_instructions().insert(it, newval);
            }
        } else if (std::abs(d) == (1ull << l)) {
            // LOG_INFO << "BB before div inv\n" << bb->print();
            // 处理向 0 舍入的问题
            // TODO: 会产生移动 0 位的指令，需要后续消除
            auto newval1 = BinaryInst::create(
                Type::get_int32_type(m_), Instruction::ashr, lhs, ConstantInt::get(l - 1, m_), nullptr);
            auto newval2 = BinaryInst::create(Type::get_int32_type(m_),
                                              Instruction::lshr,
                                              newval1,
                                              ConstantInt::get(static_cast<int>(N - l), m_),
                                              nullptr);
            auto newval3 = BinaryInst::create(Type::get_int32_type(m_), Instruction::add, lhs, newval2, nullptr);
            auto newval4 = BinaryInst::create(
                Type::get_int32_type(m_), Instruction::ashr, newval3, ConstantInt::get(l, m_), nullptr);

            newval1->set_parent(bb.get());
            newval2->set_parent(bb.get());
            newval3->set_parent(bb.get());
            newval4->set_parent(bb.get());
            // DONE 处理负数
            auto val = *it;
            (*it)->remove_use_of_ops();
            it = bb->get_instructions().erase(it);

            bb->get_instructions().insert(it, newval1);
            bb->get_instructions().insert(it, newval2);
            bb->get_instructions().insert(it, newval3);
            bb->get_instructions().insert(it, newval4);
            if (d < 0) {
                auto newval5 = BinaryInst::create(
                    Type::get_int32_type(m_), Instruction::sub, ConstantInt::get(0, m_), newval4, nullptr);
                newval5->set_parent(bb.get());
                bb->get_instructions().insert(it, newval5);
                val->replace_all_use_with(newval5);
            } else
                val->replace_all_use_with(newval4);

            // LOG_INFO << "BB before div inv\n" << bb->print();
        } else if (m < (1ull << (N - 1))) {
            // TODO: replace it with SRA(MULSH(m,n),shpost)-XSIGN(n);
            ++it;
        } else {
            // TODO: repalce it with SRA(n+MULSH(m-2^N,n),shpost)-XSIGN(n)
            ++it;
        }
    }

  private:
    static constexpr unsigned long long N = 32;
    int ins_count{0};
};

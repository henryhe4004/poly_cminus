#pragma once

#include "Pass.hh"
#include "SCCPSolver.hh"

#include <algorithm>
#include <deque>
#include <unordered_map>

/**
 * @brief 参考 llvm 实现重写的 sccp
 */
class SCCP : public Pass {
  public:
    SCCP(Module *m) : Pass(m) {}
    void run() {
        for (auto func : m_->get_functions()) {
            if (func->get_num_basic_blocks() == 0)
                continue;
            process_func(func);
        }
    }

  private:
    int ins_count{0};
    void process_func(std::shared_ptr<Function> func) {
        solver = std::make_shared<SCCPSolver>(m_, func);
        solver->mark_block_executable(func->get_entry_block());
        for (auto arg : func->get_args())
            solver->mark_overdefined(arg);
        do {
            solver->solve();
        } while (solver->process_unknown());
        solver->log();
        rewrite(func);
        LOG_INFO << "SCCP: replace " << ins_count << " instructions";
    }
    void rewrite(std::shared_ptr<Function> func) {
        for (auto bb : func->get_basic_blocks()) {
            if (not solver->is_block_executable(bb)) {
                // 不替换对应指令的话会出错
                // 清理还是交给控制流化简把
                // while (*bb->get_instructions().begin() != bb->get_terminator())
                //     (*bb->get_instructions().begin())->erase_from_parent();
                // if (auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator())) {
                //     if (brinst->is_cond_br()) {
                //         bb->remove_succ_basic_block(brinst->get_false_succ().get());
                //         brinst->get_false_succ()->remove_pre_basic_block(bb.get());
                //         brinst->set_operand(0, brinst->get_operand(1));
                //         brinst->remove_operands(1, 2);
                //     }
                // }
                continue;
            }
            for (auto inst : bb->get_instructions()) {
                if (inst->is_void())
                    continue;
                auto lat = solver->get_value_state(inst);
                if (lat.is_constant()) {
                    inst->replace_all_use_with(lat.val);
                    ++ins_count;
                    // 删除工作交给死代码删除
                    continue;
                }
                // 重写 phi 指令
                if (inst->is_phi()) {
                    for (int i = 0; i < inst->get_num_operand(); i += 2) {
                        if (not solver->is_edge_feasible(
                                std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(i + 1)->shared_from_this()),
                                bb)) {
                            inst->remove_operands(i, i + 1);
                            i -= 2;
                        }
                    }
                    // DONE：单一前驱块的 phi 指令可以被改写
                    // UPD: 在以下代码下会出问题：
                    /**
                        int a = 5;
                        int main() {
                            int b;
                            while (a)
                                while (b)
                                    b = b;
                        }

                     */
                    // if (inst->get_num_operand() == 2)
                    //     inst->replace_all_use_with(inst->get_operand(0)->shared_from_this());
                    exit_if(inst->get_num_operand() == 0, ERROR_IN_SCCP, "SCCP: zero operand inst");
                }
            }
            // for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end(); ++it) {
            //     auto inst = *it;
            //     if (inst->is_phi() == false)
            //         break;
            //     if (inst->get_num_operand() == 0) {
            //         ++it;
            //         inst->erase_from_parent();
            //     } else
            //         ++it;
            // }
            if (auto br_inst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator())) {
                if (not br_inst->is_cond_br())
                    continue;
                auto lat = solver->get_value_state(br_inst->get_operand(0)->shared_from_this());
                if (not lat.is_constant())
                    continue;
                exit_if(isa<ConstantInt>(lat.val) == false, ERROR_IN_SCCP, "SCCP: error in rewrite br inst");

                auto if_true = std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(1)->shared_from_this());
                auto if_false = std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(2)->shared_from_this());
                auto val = std::dynamic_pointer_cast<ConstantInt>(lat.val)->get_value();
                ++ins_count;
                if_false->remove_pre_basic_block(bb.get());
                bb->remove_succ_basic_block(if_false.get());
                if_true->remove_pre_basic_block(bb.get());
                bb->remove_succ_basic_block(if_true.get());
                if (val) {
                    br_inst->erase_from_parent();
                    BranchInst::create_br(if_true, bb.get());
                } else {
                    br_inst->erase_from_parent();
                    BranchInst::create_br(if_false, bb.get());
                }
            }
        }
    }
    std::shared_ptr<SCCPSolver> solver;
};

/**
 * @brief 实现稀疏条件常数传播，大概吧？
 * 参考资料：
 * 编译器设计-合并优化
 * 编译器设计-使用静态单赋值形式
 * 高级编译器设计与实现-稀有条件常数传播
 * 前置阅读：
 * 编译器设计与高级编译器设计与实现关于格的引入的部分
 * WARN: 目前不能对数组中的元素进行常量传播，因为涉及 load 和 gep 指令
 * WARN: 目前要求 alloc 指令在 bb 的开头
 */
class SCCPOld : public Pass {
  public:
    SCCPOld(Module *m) : Pass(m) {}

    void run() {
        for (auto func : m_->get_functions()) {
            if (func->get_num_basic_blocks() == 0)
                continue;

            // propagation(func);
            // print_inst_and_lattice(func);
            process_func(func);
        }
    };

  private:
    template <class T>
    static std::string to_string(const T &t) {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }
    void print_inst_and_lattice(std::shared_ptr<Function> func) {
        for (auto item : value) {
            LOG_INFO << "value is: " << item.first->print() << " ,lattice is: " << item.second.print();
        }
    }
    struct lattice {
        int state;  // 1 代表是未定的常数，0 代表是常数，-1 代表是变量
        std::shared_ptr<Constant> val;
        bool is_float{false};  // 方便确定常数值类型, TODO: 这一项可以直接判断，不需要保存，之后可以删掉这个字段
        std::string print() {
            std::string ret{};
            ret += "state: ";
            ret += to_string(state);

            if (state == 0) {
                ret += " ,value is: ";
                if (is_float)
                    ret += to_string(std::dynamic_pointer_cast<ConstantFP>(val)->get_value());
                else
                    ret += to_string(std::dynamic_pointer_cast<ConstantInt>(val)->get_value());
            }
            return ret;
        }
    };
    lattice meet(const lattice &lhs, const lattice &rhs) {
        if (lhs.state < rhs.state)
            return lhs;
        if (lhs.state > rhs.state)
            return rhs;
        if (lhs.state == rhs.state and lhs.state != 0)
            return lhs;
        if (lhs.is_float == rhs.is_float)  // 相同常数值时
        {
            if (lhs.is_float and std::dynamic_pointer_cast<ConstantFP>(lhs.val)->get_value() ==
                                     std::dynamic_pointer_cast<ConstantFP>(rhs.val)->get_value())
                return lhs;
            if (lhs.is_float == false and std::dynamic_pointer_cast<ConstantInt>(lhs.val)->get_value() ==
                                              std::dynamic_pointer_cast<ConstantInt>(rhs.val)->get_value())
                return lhs;
        }
        return {-1, nullptr};
    }
    // 现阶段基于点的传播可能在一些情况下不能正确求出常数，但在没有 goto 语句的情况下应该不会出现这种例子
    // TODO: 可能的改进：改为基于边的传播
    std::unordered_map<std::shared_ptr<Value>, lattice> value;
    std::deque<std::shared_ptr<Instruction>> SSAWorklist;
    std::deque<std::shared_ptr<Instruction>> CFGWorklist;
    std::unordered_map<std::shared_ptr<Value>, bool> executed;
    void process_func(std::shared_ptr<Function> func) {
        propagation(func);
        rewrite(func);
    }
    void set_default_lattice(std::shared_ptr<Function> func) {
        // 参数值认为已经标记过了
        for (auto arg : func->get_args()) {
            value[arg] = {-1, nullptr};  // 参数默认为不定值
            executed[arg] = true;
        }
        for (auto bb : func->get_basic_blocks())
            for (auto inst : bb->get_instructions())
                value[inst] = get_default_lattice(inst);
        // SSCP();
    }
    // void SSCP() {}
    bool is_argument(std::shared_ptr<Value> val) { return std::dynamic_pointer_cast<Argument>(val) != nullptr; }
    void propagation(std::shared_ptr<Function> func) {
        value.clear();
        // WARN: 初始值不能使用 SSCP 完成，这样会导致部分常数无法被识别
        // FIX: 因为形如 icmp slt ... 嵌套指令的存在，还是要用 SSCP 来初始化格值
        // FIX: 不通过 SSCP 初始化也行
        set_default_lattice(func);
        // print_inst_and_lattice(func);

        auto first_inst = *func->get_entry_block()->get_instructions().begin();
        CFGWorklist.push_back(first_inst);
        while (CFGWorklist.empty() == false or SSAWorklist.empty() == false) {
            if (not CFGWorklist.empty()) {
                auto now = std::dynamic_pointer_cast<Instruction>(CFGWorklist.front());
                LOG_INFO << "process CFGWorklist: " << now->print() << " " << value[now].print();
                assert(now != nullptr);
                CFGWorklist.pop_front();
                if (now->is_phi() or now->is_alloca()) {
                    CFGWorklist.push_back(get_succ(now));
                } else if (not executed[now]) {
                    executed[now] = true;
                    // 因为 phi 指令中可能有常数，这时候需要靠 bb 来确定某个值是否有影响
                    executed[now->get_parent()->shared_from_this()] = true;
                    evaluate_all_phi_in_block(now);
                    if (now->is_br()) {
                        evaluate_conditional(now);
                    } else if (is_assign(now)) {
                        evaluate_assign(now);
                        CFGWorklist.push_back(get_succ(now));
                    }
                }
            }
            if (not SSAWorklist.empty()) {
                auto now = SSAWorklist.front();
                SSAWorklist.pop_front();
                LOG_INFO << "process SSAWorklist: " << now->print() << " " << value[now].print();

                bool is_any_previous_value_executed = false;
                for (int i = 0; i < now->get_num_operand(); ++i) {
                    auto op = now->get_operand(i)->shared_from_this();
                    if (executed[op]) {
                        is_any_previous_value_executed = true;
                        break;
                    }
                }
                if (is_any_previous_value_executed) {
                    if (now->is_phi())
                        evaluate_phi(std::dynamic_pointer_cast<PhiInst>(now));
                    else if (is_assign(now))
                        evaluate_assign(now);
                    else if (now->is_br())
                        evaluate_conditional(now);
                } else {
                    LOG_WARNING << "an ssa value in ssa worklist but no previous result executed: " << now->print();
                }
            }
        }
    }
    std::shared_ptr<Instruction> get_succ(std::shared_ptr<Instruction> inst) {
        // TODO: 除了暴力遍历链表还有更好找到下一条指令的方式吗?
        auto bb = inst->get_parent();
        auto it = std::find(bb->get_instructions().begin(), bb->get_instructions().end(), inst);
        // phi 在进入块的时候就被 evaluate 掉了，因此后继要找不是 phi 的指令
        ++it;
        while ((*it)->is_phi() or (*it)->is_alloca())
            ++it;
        return *it;
    }
    lattice evaluate_inst(std::shared_ptr<Instruction> inst);
    void evaluate_all_phi_in_block(std::shared_ptr<Instruction> inst) {
        auto bb = std::dynamic_pointer_cast<BasicBlock>(inst->get_parent()->shared_from_this());
        std::unordered_map<std::shared_ptr<PhiInst>, lattice> tmp_val{};
        for (auto inst : bb->get_instructions())
            if (inst->is_phi())
                tmp_val[std::dynamic_pointer_cast<PhiInst>(inst)] =
                    evaluate_operands(std::dynamic_pointer_cast<PhiInst>(inst));
            else
                break;
        for (auto inst : bb->get_instructions())
            if (inst->is_phi())
                evaluate_results(std::dynamic_pointer_cast<PhiInst>(inst),
                                 tmp_val[std::dynamic_pointer_cast<PhiInst>(inst)]);
            else
                break;
        for (auto inst : bb->get_instructions())
            if (inst->is_phi()) {
                executed[inst] = true;
                executed[inst->get_parent()->shared_from_this()] = true;
            } else
                break;
    }
    void evaluate_assign(std::shared_ptr<Instruction> inst) {
        // 不定值，不需要计算了
        if (value[inst].state == -1)
            return;
        // 针对运算符进行格值计算
        check_inst(inst);
        lattice new_lattice = evaluate_inst(inst);
        if (new_lattice.state != value[inst].state) {
            value[inst] = new_lattice;
            for (auto use : inst->get_use_list()) {
                auto use_to_inst = std::dynamic_pointer_cast<Instruction>(use.val_->shared_from_this());
                if (use_to_inst != nullptr)
                    SSAWorklist.push_back(use_to_inst);
            }
        }
    }
    void check_inst(std::shared_ptr<Instruction> inst) {
        for (int i = 0; i < inst->get_num_operand(); ++i) {
            if (value.find(inst->get_operand(i)->shared_from_this()) == value.end()) {
                LOG_ERROR << "found inst that not stored in lattice table: " << inst->print()
                          << " , operand is: " << inst->get_operand(i)->shared_from_this()->print();
                exit(-1);
            }
        }
    }
    void evaluate_conditional(std::shared_ptr<Instruction> inst) {
        auto br_inst = std::dynamic_pointer_cast<BranchInst>(inst);
        if (br_inst->is_cond_br()) {
            auto cond = br_inst->get_operand(0)->shared_from_this();
            auto if_true = std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(1)->shared_from_this());
            auto if_false = std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(2)->shared_from_this());
            // 执行到跳转分支时，条件变量要么是常数，要么是不定值
            assert(value[cond].state != 1);
            if (value[cond].state == -1) {
                CFGWorklist.push_back(*if_true->get_instructions().begin());
                CFGWorklist.push_back(*if_false->get_instructions().begin());
                value[inst] = {-1, nullptr};
            } else if (std::dynamic_pointer_cast<ConstantInt>(value[cond].val)->get_value()) {
                CFGWorklist.push_back(*if_true->get_instructions().begin());
                value[inst] = {0, ConstantInt::get(1, m_), false};
            } else {
                CFGWorklist.push_back(*if_false->get_instructions().begin());
                value[inst] = {0, ConstantInt::get(0, m_), false};
            }
        } else {
            auto bb = std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(0)->shared_from_this());
            CFGWorklist.push_back(*bb->get_instructions().begin());
        }
    }
    // TODO: 这个查找步骤也需要优化
    std::shared_ptr<Instruction> get_inst_wanted(std::shared_ptr<Instruction> inst) {
        auto bb = inst->get_parent();
        auto it = std::find(bb->get_instructions().begin(), bb->get_instructions().end(), inst);
        while ((*it)->is_phi() or (*it)->is_alloca())
            ++it;
        return *it;
    }
    void evaluate_phi(std::shared_ptr<PhiInst> inst) {
        // 已经是不定值了就不需要更新了
        executed[inst] = true;
        if (value[inst].state == -1)
            return;
        lattice val = evaluate_operands(inst);
        evaluate_results(inst, val);
    }
    lattice evaluate_operands(std::shared_ptr<PhiInst> inst) {
        lattice val = {1, nullptr, false};
        for (int i = 0; i < inst->get_num_operand(); i += 2) {
            // FIX：phi 的操作数也可能是 Constant 而不是 Instruction
            auto op = std::dynamic_pointer_cast<Value>(inst->get_operand(i)->shared_from_this());
            auto pre_bb = std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(i + 1)->shared_from_this());
            if (op != nullptr and executed[pre_bb])
                val = meet(val, value[inst->get_operand(i)->shared_from_this()]);
        }
        return val;
    }
    void evaluate_results(std::shared_ptr<PhiInst> inst, lattice new_val) {
        if (value[inst].state == -1)
            return;
        if (value[inst].state != new_val.state) {
            value[inst] = new_val;
            for (auto use : inst->get_use_list()) {
                auto use_to_inst = std::dynamic_pointer_cast<Instruction>(use.val_->shared_from_this());
                if (use_to_inst != nullptr)
                    SSAWorklist.push_back(use_to_inst);
            }
        }
    }
    void rewrite(std::shared_ptr<Function> func) {
        // 删除指令的工作就丢给死代码消除了
        // TODO: 空BB的清理需要交给控制流化简，多条 br 的合并也需要控制流化简
        for (auto bb : func->get_basic_blocks())
            for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end(); ++it) {
                auto inst = *it;
                if (is_assign(inst)) {
                    if (value[inst].state == 0) {
                        inst->replace_all_use_with(value[inst].val);
                        inst->remove_use_of_ops();
                    }
                    if (value[inst].state == 1)
                        LOG_WARNING << "A top lattice instruction found: " << inst->print();
                } else if (inst->is_br()) {
                    auto br_inst = std::dynamic_pointer_cast<BranchInst>(inst);
                    if (br_inst->is_cond_br()) {
                        // 重写条件跳转指令
                        auto val = br_inst->get_operand(0)->shared_from_this();
                        if (value[inst].state == 1)
                            LOG_WARNING << "A top lattice value in cond br found: " << inst->print();
                        if (value[inst].state == 0) {
                            auto if_true =
                                std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(1)->shared_from_this());
                            auto if_false =
                                std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(2)->shared_from_this());

                            // 直接跳转到 true 分支
                            if (std::dynamic_pointer_cast<ConstantInt>(value[inst].val)->get_value()) {
                                it = bb->get_instructions().erase(it);
                                bb->remove_succ_basic_block(if_false.get());
                                if_false->remove_pre_basic_block(bb.get());
                                BranchInst::create_br(if_true, bb.get());
                                // 最后一条指令了，直接退出循环就行
                                break;
                            } else {
                                it = bb->get_instructions().erase(it);
                                bb->remove_succ_basic_block(if_true.get());
                                if_true->remove_pre_basic_block(bb.get());
                                BranchInst::create_br(if_false, bb.get());
                                break;
                            }
                        }
                    }
                }
            }
    }
    lattice get_default_lattice(std::shared_ptr<Value> val) {
        if (value.find(val) != value.end())
            return value[val];
        // LOG_INFO << "set default lattice for: " << val->print();
        // TODO: 可能的优化：对全局变量进行常数传播（对 load 指令做优化）
        // TODO: 可能的优化：将 gep 转化为指令加减，以便进行常数传播
        // 认为函数调用获得的值和从内存中读取的值都是不定值，gep 指令不能替换为常数，因此也认为是不定值
        if (std::dynamic_pointer_cast<ConstantInt>(val) != nullptr)
            return {0, std::dynamic_pointer_cast<Constant>(val), false};
        if (std::dynamic_pointer_cast<ConstantFP>(val) != nullptr)
            return {0, std::dynamic_pointer_cast<Constant>(val), false};
        if (std::dynamic_pointer_cast<BasicBlock>(val))
            return {1, nullptr, false};  // BB 已经事先确定了，不会降低其他指令的格值
        auto inst = std::dynamic_pointer_cast<Instruction>(val);
        if (inst == nullptr) {
            LOG_ERROR << "A value whose type is not considered: " << val->print();
            exit(-1);
        }
        if (inst->is_call() or inst->is_store() or inst->is_load() or inst->is_gep())
            return {-1, nullptr};
        if (inst->is_alloca())
            return {-1, nullptr};
        if (inst->is_ret())
            return {-1, nullptr, false};  // 返回指令的 lattice 几乎不会用到，大概？
        if (inst->is_br()) {
            if (std::dynamic_pointer_cast<BranchInst>(inst)->is_cond_br())
                return {1, nullptr, false};
            return {-1, nullptr, false};
        }

        value[inst] = {1, nullptr};  // 乐观估计，同时为了保证不会无限递归
        for (auto op : inst->get_operands()) {
            auto op_ptr = op->shared_from_this();
            if (value.find(op_ptr) != value.end())
                continue;
            value[op_ptr] = get_default_lattice(op_ptr);
        }
        if (inst->is_phi())
            return {1, nullptr};     // evaluate 里面没有 phi 指令的处理...就直接当做不确定值把
        return evaluate_inst(inst);  // inst 的每个操作数都有设定格值了，因此可以尝试 evaluate
    }
    bool is_assign(std::shared_ptr<Instruction> inst) {
        if (inst->is_void())
            return false;
        if (inst->is_alloca())
            return false;
        return true;
    }
};

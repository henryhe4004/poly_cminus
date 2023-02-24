#pragma once
#include "Module.h"
#include "Value.h"
#include "errorcode.hh"
#include "utils.hh"

#include <optional>

/**
 * @brief simple alias analysis
 *
 */

enum class AliasResult : uint8_t {
    /// The two locations do not alias at all.
    ///
    /// This value is arranged to convert to false, while all other values
    /// convert to true. This allows a boolean context to convert the result to
    /// a binary flag indicating whether there is the possibility of aliasing.
    NoAlias = 0,
    /// The two locations may or may not alias. This is the least precise
    /// result.
    MayAlias,
    /// The two locations precisely alias each other.
    MustAlias,
};

class MemAddress {
  public:
    MemAddress(Value *ptr) : ptr_(ptr) {
        std::vector<GetElementPtrInst *> gep_seq_reverse;
        base = ptr;
        auto gep_inst = dynamic_cast<GetElementPtrInst *>(ptr);
        while (gep_inst) {
            gep_seq_reverse.push_back(gep_inst);
            base = gep_inst->get_operand(0).get();
            gep_inst = dynamic_cast<GetElementPtrInst *>(gep_inst->get_operand(0).get());
        }
        gep_seq.assign(gep_seq_reverse.rbegin(), gep_seq_reverse.rend());
    }

    Value *get_base() { return base; }
    std::optional<int> get_const_offset() const {
        int offset = 0;
        for (auto gep : gep_seq) {
            auto base = gep->get_operand(0)->shared_from_this();
            std::vector<std::shared_ptr<Value>> idxs;
            for (int i = 1u; i < gep->get_num_operand(); i++) {
                auto ci = utils::get_const_int_val(gep->get_operand(i).get());
                if (!ci.has_value())
                    return std::nullopt;
                idxs.push_back(gep->get_operand(i)->shared_from_this());
                auto element_type = GetElementPtrInst::get_element_type(base, idxs);
                offset += ci.value() * element_type->get_size();
            }
        }
        return offset;
    }
    bool operator==(const MemAddress &rhs) {
        auto c_off_l = this->get_const_offset();
        auto c_off_r = rhs.get_const_offset();
        return this->base == rhs.base && c_off_l.has_value() && c_off_r.has_value() && c_off_l == c_off_r;
    }
    bool operator!=(const MemAddress &rhs) {
        auto c_off_l = this->get_const_offset();
        auto c_off_r = rhs.get_const_offset();
        return this->base != rhs.base || (c_off_l.has_value() && c_off_r.has_value() && c_off_l != c_off_r);
    }

    bool has_const_base() { return isa<AllocaInst, GlobalVariable, Argument>(base); }
    Value *get_ptr() { return ptr_; }
    std::vector<GetElementPtrInst *> &get_gep_insts() { return gep_seq; }

  private:
    Value *ptr_;
    Value *base;
    std::vector<GetElementPtrInst *> gep_seq;
};

// 简单的别名分析
class AliasAnalysis {
  public:
    // AliasAnalysis(Module *m) : m_(m) {}

    /// 返回NoAlias当：
    /// v1与v2的基址为不同的常数地址（alloca指令、全局数组、参数）
    /// v1与v2的基址为相同的常数地址，且偏移量均为常数且不相等
    ///
    /// 返回MustAlias当：
    /// v1与v2的基址为相同的常数地址，且偏移量均为常数且相等
    ///
    /// 返回MayAlias当：
    /// 所有其它情况，如
    /// v1与v2有相同常数基址，但至少有一个的偏移量不为常数
    /// v1或v2有非常数的基址，如phi
    AliasResult alias(Value *v1, Value *v2) {
        if (!v1->get_type()->is_pointer_type()) {
            LOG_ERROR << v1->print() << " has non-pointer type";
            exit(AA_TYPE_ERROR);
        }
        if (!v2->get_type()->is_pointer_type()) {
            LOG_ERROR << v2->print() << " has non-pointer type";
            exit(AA_TYPE_ERROR);
        }
        if (v1->get_type() != v2->get_type()) {
            LOG_ERROR << v1->print() << " and " << v2->print() << " have different types";
            exit(AA_TYPE_ERROR);
        }
        // AliasResult result = AliasResult::MayAlias;
        auto v1_addr = MemAddress(v1);
        auto v2_addr = MemAddress(v2);
        // auto v1_base = v1_addr.get_base();
        // auto v2_base = v2_addr.get_base();
        if (v1_addr.has_const_base() && v2_addr.has_const_base()) {
            if (v1_addr == v2_addr)
                return AliasResult::MustAlias;
            if (v1_addr != v2_addr)
                return AliasResult::NoAlias;
        }

        return AliasResult::MayAlias;
    }

  private:
    // Module *m_;
};

#pragma once
#include "Pass.hh"
#include "errorcode.hh"
#include "utils.hh"

class ConstFold {
  public:
    ConstFold(Module *m) : m_(m) {}
    std::shared_ptr<Value> eval(Instruction::OpID op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        // TODO: 添加对 0 的特判，两者不为常数时也进行计算
        if (isa<ConstantInt>(lhs) and isa<ConstantInt>(rhs)) {
            int lval = std::dynamic_pointer_cast<ConstantInt>(lhs)->get_value();
            int rval = std::dynamic_pointer_cast<ConstantInt>(rhs)->get_value();
            int ret = 0;
            switch (op) {
                case Instruction::add:
                    ret = lval + rval;
                    break;
                case Instruction::sub:
                    ret = lval - rval;
                    break;
                case Instruction::mul:
                    ret = lval * rval;
                    break;
                case Instruction::sdiv:
                    ret = rval ? lval / rval : 0;
                    break;
                case Instruction::srem:
                    ret = rval ? lval % rval : 0;
                    break;
                case Instruction::shl:
                    ret = (lval << rval);
                    break;
                case Instruction::lshr:
                    ret = ((static_cast<unsigned>(lval)) >> (static_cast<unsigned>(rval)));
                    break;
                case Instruction::ashr:
                    ret = (lval >> rval);
                    break;
                case Instruction::and_:
                    ret = (lval & rval);
                    break;
                default:
                    exit(ERROR_IN_CONST_FOLD, "ConstFold: unhandled eval type" + std::to_string(op));
            }
            if (std::dynamic_pointer_cast<ConstantInt>(lhs)->get_type()->get_size() != 8)
                return ConstantInt::get(ret, m_);
            return ConstantInt::get_i64(ret, m_);
        }
        if (isa<ConstantFP>(lhs) and isa<ConstantFP>(rhs)) {
            float lval = std::dynamic_pointer_cast<ConstantFP>(lhs)->get_value();
            float rval = std::dynamic_pointer_cast<ConstantFP>(rhs)->get_value();
            float ret = 0;
            switch (op) {
                case Instruction::fadd:
                    ret = lval + rval;
                    break;
                case Instruction::fsub:
                    ret = lval - rval;
                    break;
                case Instruction::fmul:
                    ret = lval * rval;
                    break;
                case Instruction::fdiv:
                    ret = lval / rval;
                    break;
                default:
                    exit(ERROR_IN_CONST_FOLD, "ConstFold: unhandled eval type" + std::to_string(op));
            }
            return ConstantFP::get(ret, m_);
        }
        return nullptr;
    }
    std::shared_ptr<Value> eval(CmpOp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        if (isa<ConstantInt>(lhs) and isa<ConstantInt>(rhs)) {
            int lval = std::dynamic_pointer_cast<ConstantInt>(lhs)->get_value();
            int rval = std::dynamic_pointer_cast<ConstantInt>(rhs)->get_value();
            bool ret = 0;
            switch (op) {
                case CmpOp::EQ:
                    ret = (lval == rval);
                    break;
                case CmpOp::NE:
                    ret = (lval != rval);
                    break;
                case CmpOp::GT:
                    ret = (lval > rval);
                    break;
                case CmpOp::GE:
                    ret = (lval >= rval);
                    break;
                case CmpOp::LT:
                    ret = (lval < rval);
                    break;
                case CmpOp::LE:
                    ret = (lval <= rval);
                    break;
            }
            return ConstantInt::get(ret, m_);
        }
        if (isa<ConstantFP>(lhs) and isa<ConstantFP>(rhs)) {
            float lval = std::dynamic_pointer_cast<ConstantFP>(lhs)->get_value();
            float rval = std::dynamic_pointer_cast<ConstantFP>(rhs)->get_value();
            bool ret = 0;
            switch (op) {
                case CmpOp::EQ:
                    ret = (lval == rval);
                    break;
                case CmpOp::NE:
                    ret = (lval != rval);
                    break;
                case CmpOp::GT:
                    ret = (lval > rval);
                    break;
                case CmpOp::GE:
                    ret = (lval >= rval);
                    break;
                case CmpOp::LT:
                    ret = (lval < rval);
                    break;
                case CmpOp::LE:
                    ret = (lval <= rval);
                    break;
            }
            return ConstantInt::get(ret, m_);
        }
        if (isa<ConstantFP>(lhs) and isa<ConstantInt>(rhs)) {
            float lval = std::dynamic_pointer_cast<ConstantFP>(lhs)->get_value();
            int rval = std::dynamic_pointer_cast<ConstantInt>(rhs)->get_value();
            bool ret = 0;
            switch (op) {
                case CmpOp::EQ:
                    ret = (lval == rval);
                    break;
                case CmpOp::NE:
                    ret = (lval != rval);
                    break;
                case CmpOp::GT:
                    ret = (lval > rval);
                    break;
                case CmpOp::GE:
                    ret = (lval >= rval);
                    break;
                case CmpOp::LT:
                    ret = (lval < rval);
                    break;
                case CmpOp::LE:
                    ret = (lval <= rval);
                    break;
            }
            return ConstantInt::get(ret, m_);
        }
        if (isa<ConstantInt>(lhs) and isa<ConstantFP>(rhs)) {
            int lval = std::dynamic_pointer_cast<ConstantInt>(lhs)->get_value();
            float rval = std::dynamic_pointer_cast<ConstantFP>(rhs)->get_value();
            bool ret = 0;
            switch (op) {
                case CmpOp::EQ:
                    ret = (lval == rval);
                    break;
                case CmpOp::NE:
                    ret = (lval != rval);
                    break;
                case CmpOp::GT:
                    ret = (lval > rval);
                    break;
                case CmpOp::GE:
                    ret = (lval >= rval);
                    break;
                case CmpOp::LT:
                    ret = (lval < rval);
                    break;
                case CmpOp::LE:
                    ret = (lval <= rval);
                    break;
            }
            return ConstantInt::get(ret, m_);
        }
        return nullptr;
    }

  private:
    Module *m_;
};

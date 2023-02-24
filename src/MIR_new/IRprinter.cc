#include "IRprinter.h"

std::string print_as_op(Value *v, bool print_ty) {
    std::string op_ir;
    if (print_ty) {
        if (auto g = dynamic_cast<GlobalVariable *>(v)) {
            op_ir += g->get_init()->get_type()->print();
            op_ir += "*";
        } else
            op_ir += v->get_type()->print();
        op_ir += " ";
    }

    if (dynamic_cast<GlobalVariable *>(v)) {
        op_ir += "@" + v->get_name();
    } else if (dynamic_cast<Function *>(v)) {
        op_ir += "@" + v->get_name();
    } else if (dynamic_cast<Constant *>(v)) {
        op_ir += v->print();
    } else {
        op_ir += "%" + v->get_name();
    }

    return op_ir;
}

std::string print_cmp_type(CmpOp op) {
    switch (op) {
        case CmpOp::GE:
            return "sge";
            break;
        case CmpOp::GT:
            return "sgt";
            break;
        case CmpOp::LE:
            return "sle";
            break;
        case CmpOp::LT:
            return "slt";
            break;
        case CmpOp::EQ:
            return "eq";
            break;
        case CmpOp::NE:
            return "ne";
            break;
        default:
            break;
    }
    return "wrong cmpop";
}

std::string print_fcmp_type(CmpOp op) {
    switch (op) {
        case CmpOp::GE:
            return "uge";
            break;
        case CmpOp::GT:
            return "ugt";
            break;
        case CmpOp::LE:
            return "ule";
            break;
        case CmpOp::LT:
            return "ult";
            break;
        case CmpOp::EQ:
            return "ueq";
            break;
        case CmpOp::NE:
            return "une";
            break;
        default:
            break;
    }
    return "wrong fcmpop";
}
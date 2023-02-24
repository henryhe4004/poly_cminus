#include "lir.h"

std::string MovInst::print() {
    std::string lir;
    lir += get_module()->get_instr_op_name(get_instr_type());
    lir += " ";
    lir += this->get_operand(0)->get_type()->print();
    lir += " ";
    lir += print_as_op(get_operand(0).get(), false);
    lir += " ";
    lir += this->get_operand(1)->get_type()->print();
    lir += " ";
    lir += print_as_op(get_operand(1).get(), false);
    return lir;
}

std::string MuladdInst::print() {
    std::string lir;
    lir += "%";
    lir += this->get_name();
    lir += " = ";
    lir += this->get_module()->get_instr_op_name(this->get_instr_type());
    lir += " ";
    lir += this->get_operand(0)->get_type()->print();
    lir += " ";
    lir += print_as_op(get_operand(0).get(), false);
    lir += ", ";
    lir += print_as_op(get_operand(1).get(), false);
    lir += ", ";
    lir += print_as_op(get_operand(2).get(), false);
    return lir;
}

std::string MulsubInst::print() {
    std::string lir;
    lir += "%";
    lir += this->get_name();
    lir += " = ";
    lir += this->get_module()->get_instr_op_name(this->get_instr_type());
    lir += " ";
    lir += this->get_operand(0)->get_type()->print();
    lir += " ";
    lir += print_as_op(get_operand(0).get(), false);
    lir += ", ";
    lir += print_as_op(get_operand(1).get(), false);
    lir += ", ";
    lir += print_as_op(get_operand(2).get(), false);
    return lir;
}
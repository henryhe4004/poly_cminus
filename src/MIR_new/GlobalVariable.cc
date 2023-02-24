//
// Created by cqy on 2020/6/29.
//
#include "GlobalVariable.h"

#include "IRprinter.h"

GlobalVariable::GlobalVariable(std::string name, Module *m, Type *ty, bool is_const, std::shared_ptr<Constant> init)
    : User(ty, name, init != nullptr), is_const_(is_const), init_val_(init) {}  // global操作数为initval

std::shared_ptr<GlobalVariable> GlobalVariable::create(std::string name,
                                                       Module *m,
                                                       Type *ty,
                                                       bool is_const,
                                                       std::shared_ptr<Constant> init = nullptr) {
    auto ptr = std::shared_ptr<GlobalVariable>(new GlobalVariable(name, m, PointerType::get(ty), is_const, init));
    m->add_global_variable(ptr);
    if (init) {
        ptr->set_operand(0, std::static_pointer_cast<Value>(init));
    }
    return ptr;
}

std::string GlobalVariable::print() {
    std::string global_val_ir;
    global_val_ir += print_as_op(this, false);
    global_val_ir += " = ";
    global_val_ir += (this->is_const() ? "constant " : "global ");
    global_val_ir += this->get_init()->get_type()->print();
    global_val_ir += " ";
    global_val_ir += this->get_init()->print_initializer();
    return global_val_ir;
}

#include "User.h"

#include "Instruction.h"

#include <cassert>

User::User(Type *ty, const std::string &name, unsigned num_ops) : Value(ty, name), num_ops_(num_ops) {
    // if (num_ops_ > 0)
    //   operands_.reset(new std::list<std::shared_ptr<Value>>());
    operands_.resize(num_ops_, nullptr);
}

std::vector<User::operand_t> &User::get_operands() { return operands_; }

const User::operand_t &User::get_operand(unsigned i) const { return operands_[i]; }

User::operand_t::operand_t(std::shared_ptr<Instruction> p)
    : operand(std::weak_ptr(std::static_pointer_cast<Value>(p))) {}
void User::set_operand(unsigned i, operand_t v) {
    assert(i < num_ops_ && "set_operand out of index");
    // assert(operands_[i] == nullptr && "ith operand is not null");
    operands_[i] = v;
    v->add_use(this, i);
}

void User::add_operand(operand_t v) {
    operands_.push_back(v);
    v->add_use(this, num_ops_);
    num_ops_++;
}

unsigned User::get_num_operand() const { return num_ops_; }

void User::remove_use_of_ops() {
    for (auto &op : operands_) {
        op->remove_use(this);
    }
}

void User::remove_operands(int index1, int index2) {
    for (int i = index1; i < num_ops_; i++) {
        operands_[i]->remove_use(this, i);
        if (i > index2)
            operands_[i]->add_use(this, i - (index2 - index1 + 1));
    }
    operands_.erase(operands_.begin() + index1, operands_.begin() + index2 + 1);
    // std::cout<<operands_.size()<<std::endl;
    num_ops_ = operands_.size();
}

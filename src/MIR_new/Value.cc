#include "Value.h"

#include "Constant.h"
#include "Instruction.h"
#include "Type.h"
#include "User.h"

#include <cassert>

Value::Value(Type *ty, const std::string &name) : type_(ty), name_(name) {}

void Value::add_use(Value *val, unsigned arg_no) { use_list_.push_back(Use(val, arg_no)); }

std::string Value::get_name() const { return name_; }

void Value::replace_all_use_with(std::shared_ptr<Value> new_val) {
    for (auto use : use_list_) {
        auto val = dynamic_cast<User *>(use.val_);
        assert(val);
        auto rand = val->get_operand(use.arg_no_);
        if (rand.is_weak() and not std::dynamic_pointer_cast<Constant>(new_val))
            val->set_operand(use.arg_no_, std::weak_ptr(new_val));
        else
            val->set_operand(use.arg_no_, new_val);
    }
    use_list_.clear();
}

void Value::replace_use_with_when(std::shared_ptr<Value> new_val, std::function<bool(User *)> pred) {
    for (auto useit = use_list_.begin(); useit != use_list_.end();) {
        auto use = *useit;
        auto val = dynamic_cast<User *>(use.val_);
        assert(val && "new_val is not a user");
        if (not pred(val)) {
            ++useit;
            continue;
        }
        auto rand = val->get_operand(use.arg_no_);
        if (rand.is_weak() and not std::dynamic_pointer_cast<Constant>(new_val))
            val->set_operand(use.arg_no_, std::weak_ptr(new_val));
        else
            val->set_operand(use.arg_no_, new_val);
        useit = use_list_.erase(useit);
    }
}

void Value::remove_use(Value *val) {
    auto is_val = [val](const Use &use) { return use.val_ == val; };
    use_list_.remove_if(is_val);
}

void Value::remove_use(Value *val, unsigned no) {
    auto is_val = [val, no](const Use &use) { return use.val_ == val && use.arg_no_ == no; };
    use_list_.remove_if(is_val);
}

std::string Value::print_usage() {
    std::string ret = " Usage of " + print() + " :";
    for (auto use : get_use_list())
        ret += "( " + std::to_string(use.arg_no_) + " , " + use.val_->print() + " ) ";
    return ret;
}
#ifndef SYSYC_VALUE_H
#define SYSYC_VALUE_H

#include "logging.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
class Type;
class Value;
class User;
class Constant;
struct Use {
    Value *val_;
    unsigned arg_no_;  // the no. of operand, e.g., func(a, b), a is 0, b is 1
    Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}
    friend bool operator==(const Use &lhs, const Use &rhs) {
        return lhs.val_ == rhs.val_ && lhs.arg_no_ == rhs.arg_no_;
    }
};

class UseHash {
  public:
    size_t operator()(const Use &u) const {
        return (std::hash<Value *>()(u.val_)) ^ (std::hash<unsigned>()(u.arg_no_));
    }
};

class Value : public std::enable_shared_from_this<Value> {
  public:
    explicit Value(Type *ty, const std::string &name = "");
    virtual ~Value() = default;

    Type *get_type() const { return type_; }

    std::list<Use> &get_use_list() { return use_list_; }

    void add_use(Value *val, unsigned arg_no = 0);
    std::string print_usage();

    bool set_name(std::string name) {
        if (name_ == "") {
            name_ = name;
            return true;
        }
        return false;
    }
    void take_name(Value *v) {
        name_ = v->get_name();
        v->name_ = "";
    }
    std::string get_name() const;

    void replace_all_use_with(std::shared_ptr<Value> new_val);
    /// \brief replace use of `val` with `new_val` when `pred` returns true
    /// \tparam P predicate type, unary function
    void replace_use_with_when(std::shared_ptr<Value> new_val, std::function<bool(User*)> pred);
    void remove_use(Value *val);
    void remove_use(Value *val, unsigned no);

    virtual std::string print() = 0;

  protected:
    Type *type_;

  private:
    // User's `operands_` maintains a smart pointer to its operand
    // so use_list_(i.e. Use) should not hold a shared_ptr back to its user (otherwise shared_ptr won't release it), use
    // raw pointer or weak_ptr
    std::list<Use> use_list_;  // who use this value
    std::string name_;         // should we put name field here ?
};

#endif  // SYSYC_VALUE_H

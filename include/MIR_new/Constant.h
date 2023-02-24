//
// Created by cqy on 2020/6/29.
//

#ifndef SYSYC_CONSTANT_H
#define SYSYC_CONSTANT_H
#include "Type.h"
#include "User.h"
#include "Value.h"
#include "unordered_map"

class Constant : public User {
  private:
    // int value;
  public:
    Constant(Type *ty, const std::string &name = "", unsigned num_ops = 0) : User(ty, name, num_ops) {}
    ~Constant() = default;
    virtual std::string print_initializer() = 0;
    virtual bool is_constant_int() const { return false; }
    virtual bool is_constant_fp() const { return false; }
    virtual bool is_constant_array() const { return false; }
    virtual bool is_constant_zero() const { return false; }
    virtual bool is_constant_struct() const { return false; }
};

class ConstantInt : public Constant {
  private:
    int value_;
    ConstantInt(Type *ty, int val) : Constant(ty, "", 0), value_(val) {}

  public:
    static int get_value(std::shared_ptr<ConstantInt> const_val) { return const_val->value_; }
    static int get_value(ConstantInt *const_val) { return const_val->value_; }
    int get_value() { return value_; }
    static std::shared_ptr<ConstantInt> get(int val, Module *m);
    static std::shared_ptr<ConstantInt> get(bool val, Module *m);
    static std::shared_ptr<ConstantInt> get_i64(long val, Module *m);
    virtual std::string print() override;
    virtual std::string print_initializer() override { return print(); };
    virtual bool is_constant_int() const override { return true; }
};

class ConstantArray : public Constant {
  private:
    std::vector<std::shared_ptr<Constant>> const_array;

    ConstantArray(ArrayType *ty, const std::vector<std::shared_ptr<Constant>> &val);

  public:
    ~ConstantArray() = default;

    std::shared_ptr<Constant> get_element_value(int index);

    unsigned get_size_of_array() { return const_array.size(); }
    std::vector<std::shared_ptr<Constant>> get_value() { return const_array; }
    static std::shared_ptr<ConstantArray> get(ArrayType *ty, const std::vector<std::shared_ptr<Constant>> &val);

    virtual std::string print() override;

    virtual std::string print_initializer() override;
    virtual bool is_constant_array() const override { return true; }
};

class ConstantZero : public Constant {
  private:
    ConstantZero(Type *ty) : Constant(ty, "", 0) {}

  public:
    static std::shared_ptr<ConstantZero> get(Type *ty, Module *m);
    virtual std::string print() override;
    virtual std::string print_initializer() override { return print(); };
    virtual bool is_constant_zero() const override { return true; }
};

class ConstantFP : public Constant {
  private:
    float val_;
    ConstantFP(Type *ty, float val) : Constant(ty, "", 0), val_(val) {}

  public:
    static std::shared_ptr<ConstantFP> get(float val, Module *m);
    float get_value() { return val_; }
    virtual std::string print() override;
    virtual std::string print_initializer() override { return print(); };
    virtual bool is_constant_fp() const override { return true; }
};

class ConstantStruct : public Constant {
  private:
    std::vector<std::shared_ptr<Constant>> constant_struct;
    ConstantStruct(StructType *ty, const std::vector<std::shared_ptr<Constant>> &val);

  public:
    virtual std::string print() override;
    virtual std::string print_initializer() override;
    std::shared_ptr<Constant> get_element_val(unsigned idx) { return constant_struct[idx]; }
    std::vector<std::shared_ptr<Constant>> &get_elements() { return constant_struct; }
    static std::shared_ptr<ConstantStruct> get(StructType *ty, const std::vector<std::shared_ptr<Constant>> &val) {
        return std::shared_ptr<ConstantStruct>(new ConstantStruct(ty, val));
    }
    virtual bool is_constant_struct() const override { return true; }
};

#endif  // SYSYC_CONSTANT_H

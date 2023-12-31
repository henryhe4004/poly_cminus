#ifndef SYSYC_FUNCTION_H
#define SYSYC_FUNCTION_H

#include "User.h"
#include "errorcode.hh"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <memory>
class Module;
class Argument;
class BasicBlock;
class Type;
class FunctionType;

class Function : public Value {
  public:
    Function(FunctionType *ty, const std::string &name, Module *parent);
    ~Function() = default;
    static std::shared_ptr<Function> create(FunctionType *ty, const std::string &name, Module *parent);

    FunctionType *get_function_type() const;

    Type *get_return_type() const;

    void add_basic_block(std::shared_ptr<BasicBlock> bb);
    void remove_unreachable_basic_block(std::shared_ptr<BasicBlock> bb);
    void remove_basic_block(std::shared_ptr<BasicBlock> bb) { remove_unreachable_basic_block(bb); }

    unsigned get_num_of_args() const;
    unsigned get_num_basic_blocks() const;

    Module *get_parent() const;

    std::list<std::shared_ptr<Argument>>::iterator arg_begin() { return arguments_.begin(); }
    std::list<std::shared_ptr<Argument>>::iterator arg_end() { return arguments_.end(); }

    void remove(std::shared_ptr<BasicBlock> bb);
    void remove_not_bb(std::shared_ptr<BasicBlock> bb);
    std::shared_ptr<BasicBlock> get_entry_block() { return *basic_blocks_.begin(); }
    std::shared_ptr<BasicBlock> get_term_block();

    std::list<std::shared_ptr<BasicBlock>> &get_basic_blocks() { return basic_blocks_; }
    std::list<std::shared_ptr<Argument>> &get_args() { return arguments_; }

    bool is_declaration() { return basic_blocks_.empty(); }

    void set_instr_name();
    std::string print();

    auto begin() { return get_basic_blocks().begin(); }
    auto end() { return get_basic_blocks().end(); }
    // const bool has_fcalls() const;

  private:
    void build_args();

  private:
    std::list<std::shared_ptr<BasicBlock>> basic_blocks_;  // basic blocks
    std::list<std::shared_ptr<Argument>> arguments_;       // arguments
    Module *parent_;
    unsigned seq_cnt_;
    // unsigned num_args_;
    // We don't need this, all value inside function should be unnamed
    // std::map<std::string, Value*> sym_table_;   // Symbol table of args/instructions
};

// Argument of Function, does not contain actual value
class Argument : public Value {
  public:
    /// Argument constructor.
    explicit Argument(Type *ty, const std::string &name = "", Function *f = nullptr, unsigned arg_no = 0)
        : Value(ty, name), parent_(f), arg_no_(arg_no) {}
    ~Argument() {}

    inline const Function *get_parent() const { return parent_; }
    inline Function *get_parent() { return parent_; }

    /// For example in "void foo(int a, float b)" a is 0 and b is 1.
    unsigned get_arg_no() const {
        assert(parent_ && "can't get number of unparented arg");
        return arg_no_;
    }

    virtual std::string print() override;

  private:
    Function *parent_;
    unsigned arg_no_;  // argument No.
};

#endif  // SYSYC_FUNCTION_H

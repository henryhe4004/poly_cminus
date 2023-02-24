#ifndef SYSYC_MODULE_H
#define SYSYC_MODULE_H

#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Type.h"
#include "Value.h"

#include <list>
#include <map>
#include <memory>
#include <string>
class GlobalVariable;
class Module {
  public:
    explicit Module(std::string name);
    ~Module();
    Type *get_ptrtoint_type() {
        switch (target_ptr_size) {
            case 8:
                return get_int64_type();
            case 4:
                return get_int32_type();
            default:
                return nullptr;
        }
    }
    enum IR {MIR,LIR};
    IR level = MIR;
    Type *get_void_type();
    Type *get_label_type();
    IntegerType *get_int1_type();
    IntegerType *get_int32_type();
    IntegerType *get_int64_type();
    PointerType *get_int32_ptr_type();
    FloatType *get_float_type();
    PointerType *get_float_ptr_type();

    PointerType *get_pointer_type(Type *contained);
    ArrayType *get_array_type(Type *contained, unsigned num_elements);
    FunctionType *get_function_type(Type *retty, std::vector<Type *> &args);
    StructType *get_struct_type(std::vector<Type *> &contained);
    void add_function(std::shared_ptr<Function> f);
    std::list<std::shared_ptr<Function>> &get_functions();
    void add_global_variable(std::shared_ptr<GlobalVariable> g);
    std::list<std::shared_ptr<GlobalVariable>> &get_global_variables();
    std::string get_instr_op_name(Instruction::OpID instr) { return instr_id2string_[instr]; }
    void set_print_name();
    std::string get_module_name() { return module_name_; }
    void set_module_name(std::string module_name) { module_name_ = module_name; }
    std::string get_filename() { return source_filename_; }
    void set_filename(std::string filename) { source_filename_ = filename; }
    void set_ptr_size(size_t s) { target_ptr_size = s; }
    std::string print(bool skip_func_decl = false);

  private:
    std::list<std::shared_ptr<GlobalVariable>> global_list_;    // The Global Variables in the module
    std::list<std::shared_ptr<Function>> function_list_;        // The Functions in the module
    std::map<std::string, Value *> value_sym_;                  // Symbol table for values
    std::map<Instruction::OpID, std::string> instr_id2string_;  // Instruction from opid to string

    std::string module_name_;      // Human readable identifier for the module
    std::string source_filename_;  // Original source file name for module, for test and debug

  private:
    std::unique_ptr<IntegerType> int1_ty_;
    std::unique_ptr<IntegerType> int32_ty_;
    std::unique_ptr<IntegerType> int64_ty_;
    std::unique_ptr<Type> label_ty_;
    std::unique_ptr<Type> void_ty_;
    std::unique_ptr<FloatType> float32_ty_;
    size_t target_ptr_size = 8;
    std::map<Type *, std::unique_ptr<PointerType>> pointer_map_;
    std::map<std::pair<Type *, int>, std::unique_ptr<ArrayType>> array_map_;
    std::map<std::pair<Type *, std::vector<Type *>>, std::unique_ptr<FunctionType>> function_map_;
    std::map<std::vector<Type *>, std::unique_ptr<StructType>> struct_map_;
};

#endif  // SYSYC_MODULE_H
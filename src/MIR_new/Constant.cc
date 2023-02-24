#include "Constant.h"

#include "Module.h"

#include <iostream>
#include <sstream>

// DONE: 确定 constant array 可不可以缓存
// Constant Array 应该不需要缓存，但是可以通过常量传播来优化
struct pair_hash {
    template <typename T>
    std::size_t operator()(const std::pair<T, Module *> val) const {
        auto lhs = std::hash<T>()(val.first);
        auto rhs = std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(val.second));
        return lhs ^ rhs;
    }
};

static std::unordered_map<std::pair<int, Module *>, std::shared_ptr<ConstantInt>, pair_hash> cached_int;
static std::unordered_map<std::pair<bool, Module *>, std::shared_ptr<ConstantInt>, pair_hash> cached_bool;
static std::unordered_map<std::pair<long, Module *>, std::shared_ptr<ConstantInt>, pair_hash> cached_long;
static std::unordered_map<std::pair<float, Module *>, std::shared_ptr<ConstantFP>, pair_hash> cached_float;

// DONE: 同一个常数占用的内存地址可能不一样，需要讨论需不需要修改
std::shared_ptr<ConstantInt> ConstantInt::get(int val, Module *m) {
    if (cached_int.find(std::make_pair(val, m)) != cached_int.end())
        return cached_int[std::make_pair(val, m)];
    return cached_int[std::make_pair(val, m)] =
               std::shared_ptr<ConstantInt>(new ConstantInt(Type::get_int32_type(m), val));
}
std::shared_ptr<ConstantInt> ConstantInt::get(bool val, Module *m) {
    if (cached_bool.find(std::make_pair(val, m)) != cached_bool.end())
        return cached_bool[std::make_pair(val, m)];
    return cached_bool[std::make_pair(val, m)] =
               std::shared_ptr<ConstantInt>(new ConstantInt(Type::get_int1_type(m), val ? 1 : 0));
}

std::shared_ptr<ConstantInt> ConstantInt::get_i64(long val, Module *m) {
    if (cached_long.find(std::make_pair(val, m)) != cached_long.end())
        return cached_long[std::make_pair(val, m)];
    return cached_long[std::make_pair(val, m)] =
               std::shared_ptr<ConstantInt>(new ConstantInt(Type::get_int64_type(m), val));
}

std::string ConstantInt::print() {
    std::string const_ir;
    Type *ty = this->get_type();
    if (ty->is_integer_type() && static_cast<IntegerType *>(ty)->get_num_bits() == 1) {
        // int1
        const_ir += (this->get_value() == 0) ? "false" : "true";
    } else {
        // int32
        const_ir += std::to_string(this->get_value());
    }
    return const_ir;
}

ConstantArray::ConstantArray(ArrayType *ty, const std::vector<std::shared_ptr<Constant>> &val)
    : Constant(ty, "", val.size()) {
    for (int i = 0; i < val.size(); i++)
        set_operand(i, std::static_pointer_cast<Value>(val[i]));
    this->const_array.assign(val.begin(), val.end());
}

std::shared_ptr<Constant> ConstantArray::get_element_value(int index) { return this->const_array[index]; }

std::shared_ptr<ConstantArray> ConstantArray::get(ArrayType *ty, const std::vector<std::shared_ptr<Constant>> &val) {
    return std::shared_ptr<ConstantArray>(new ConstantArray(ty, val));
}

std::string ConstantArray::print() {
    std::string const_ir;
    const_ir += this->get_type()->print();
    const_ir += " ";
    const_ir += "[";
    for (int i = 0; i < this->get_size_of_array(); i++) {
        std::shared_ptr<Constant> element = get_element_value(i);
        if (!std::dynamic_pointer_cast<ConstantArray>(get_element_value(i))) {
            const_ir += element->get_type()->print();
        }
        const_ir += " ";
        const_ir += element->print();
        if (i < this->get_size_of_array() - 1) {
            const_ir += ", ";
        }
    }
    const_ir += "]";
    return const_ir;
}

std::string ConstantArray::print_initializer() {
    std::string const_ir;
    // const_ir += this->get_type()->print();
    // const_ir += " ";
    const_ir += "[";
    for (int i = 0; i < this->get_size_of_array(); i++) {
        std::shared_ptr<Constant> element = get_element_value(i);
        if (!std::dynamic_pointer_cast<ConstantArray>(get_element_value(i))) {
            const_ir += element->get_type()->print();
        }
        const_ir += " ";
        const_ir += element->print();
        if (i < this->get_size_of_array() - 1) {
            const_ir += ", ";
        }
    }
    const_ir += "]";
    return const_ir;
}

std::shared_ptr<ConstantFP> ConstantFP::get(float val, Module *m) {
    if (cached_float.find(std::make_pair(val, m)) != cached_float.end())
        return cached_float[std::make_pair(val, m)];
    return cached_float[std::make_pair(val, m)] =
               std::shared_ptr<ConstantFP>(new ConstantFP(Type::get_float_type(m), val));
}

std::string ConstantFP::print() {
    std::stringstream fp_ir_ss;
    std::string fp_ir;
    double val = this->get_value();
    fp_ir_ss << "0x" << std::hex << *(uint64_t *)&val << std::endl;
    // std::cout << "0d" << std::dec << *(uint64_t *)&val << std::endl;
    fp_ir_ss >> fp_ir;
    return fp_ir;
}

std::shared_ptr<ConstantZero> ConstantZero::get(Type *ty, Module *m) {
    return std::shared_ptr<ConstantZero>(new ConstantZero(ty));
}

std::string ConstantZero::print() { return "zeroinitializer"; }

ConstantStruct::ConstantStruct(StructType *ty, const std::vector<std::shared_ptr<Constant>> &val)
    : Constant(ty, "", val.size()) {
    for (int i = 0; i < val.size(); i++) {
        if (val[i]->get_type() != ty->get_contained_type(i)) {
            // LOG_ERROR << "error while creating constant struct";
            exit(235);
        }
        set_operand(i, std::static_pointer_cast<Value>(val[i]));
    }
    constant_struct.assign(val.begin(), val.end());
}

std::string ConstantStruct::print_initializer() {
    std::string const_ir;
    const_ir += " <{ ";
    for (int i = 0; i < get_elements().size(); i++) {
        auto ele = get_element_val(i);
        if (!(ele->is_constant_array() || ele->is_constant_struct()) || ele->is_constant_zero()) {
            const_ir += ele->get_type()->print();
            const_ir += " ";
        }
        const_ir += ele->print();
        if (i < get_elements().size() - 1)
            const_ir += ", ";
    }
    const_ir += " }>";
    return const_ir;
}

std::string ConstantStruct::print() {
    std::string const_ir;
    const_ir += type_->print();
    const_ir += " <{ ";
    for (int i = 0; i < get_elements().size(); i++) {
        auto ele = get_element_val(i);
        if (!(ele->is_constant_array() || ele->is_constant_struct()) || ele->is_constant_zero()) {
            const_ir += ele->get_type()->print();
            const_ir += " ";
        }
        const_ir += ele->print();
        if (i < get_elements().size() - 1)
            const_ir += ", ";
    }
    const_ir += " }>";
    return const_ir;
}

#pragma once
#include "Constant.h"
#include "Instruction.h"
#include "User.h"

#include <ostream>
#include <string>
#include <variant>

struct Reg {
    enum class ID : int { sp = 13, lr, pc };
    int id;
    bool is_float;
    shift_type_t op = shift_type_t::LSL;
    int shift_n = 0;

    explicit Reg(int id = -1, bool is_float = false) : id(id), is_float(is_float) {}
    Reg(Reg::ID id) : id(static_cast<int>(id)), is_float(false) {}

    std::string get_name() const {
        if (not is_float)
            return "Reg" + std::to_string(id);
        else
            return "FReg" + std::to_string(id);
    }
    const bool operator<(const Reg &rhs) const {
        return this->id < rhs.id || (this->id == rhs.id && !this->is_float && rhs.is_float);
    }
    const bool valid() const { return id != -1; }
    // bool is_reg() const override { return true; }

    bool operator==(const Reg &rhs) const { return id == rhs.id and is_float == rhs.is_float; }
    bool operator!=(const Reg &rhs) const { return !(*this == rhs); }
    bool lsl0() const { return op == shift_type_t::LSL and shift_n == 0; }
    friend std::ostream &operator<<(std::ostream &os, const Reg &r) {
        if (not r.is_float) {
            switch (r.id) {
                case 13:
                    return os << "sp";
                case 14:
                    return os << "lr";
                case 15:
                    return os << "pc";
            }
            os << "r" << r.id;
        } else {
            os << "s" << r.id;
        }
        if (r.lsl0())
            return os;
        switch (r.op) {
            case shift_type_t::LSL:
                return os << ", lsl" << " #" << r.shift_n;
            case shift_type_t::ASR:
                return os << ", asr" << " #" << r.shift_n;
            case shift_type_t::LSR:
                return os << ", lsr" << " #" << r.shift_n;
            default:
                LOG_ERROR << " ";
        }
    }
};
constexpr size_t int_variant_index = 2;
constexpr size_t float_variant_index = 3;

using armval = std::variant<Reg, Value *, size_t, size_t>;

std::ostream &operator<<(std::ostream &os, const armval &val);

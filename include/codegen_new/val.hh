#pragma once
#include "Constant.h"
#include "Instruction.h"
#include "User.h"

#include <ostream>
#include <string>
#include <variant>

struct Reg {
    // $r0          $zero       constant 0
    // $r1          $ra         return address
    // $r2          $tp         thread pointer
    // $r3          $sp         stack pointer
    // $r4 - $r5    $a0 - $a1   argument, return value
    // $r6 - $r11   $a2 - $a7   argument
    // $r12 - $r20  $t0 - $t8   temporary
    // $r21                     saved
    // $r22         $fp         frame pointer
    // $r23 - $r31  $s0 - $s8   static

    // $scr2
    // $scr3

    // $f0 - $f7    $fa0 - $fa7
    // $f8 - $f23   $ft0 - $ft15
    // $f24 - $f31  $fs0 - $fs7

    enum class ID : int { zero = -4, ra, tp, sp, ret, fp = 18 };
    int id;
    bool is_float;
    shift_type_t op = shift_type_t::LSL;
    int shift_n = 0;

    explicit Reg(int id = -5, bool is_float = false) : id(id), is_float(is_float) {}
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
    const bool valid() const { return id != -5; }
    // bool is_reg() const override { return true; }

    bool operator==(const Reg &rhs) const { return id == rhs.id and is_float == rhs.is_float; }
    bool operator!=(const Reg &rhs) const { return !(*this == rhs); }
    bool lsl0() const { return op == shift_type_t::LSL and shift_n == 0; }
    friend std::ostream &operator<<(std::ostream &os, const Reg &r) {
        if (r.is_float) {
            int id = r.id;
            if (0 <= id and id <= 7)
                return os << "$fa" << id;
            else if (8 <= id and id <= 23)
                return os << "$ft" << (id - 8);
            else if (24 <= id and id <= 31)
                return os << "$fs" << (id - 24);
            else
                abort();
        } else {
            int id = r.id + 4;
            switch (id) {
                case 0:
                    return os << "$zero";
                case 1:
                    return os << "$ra";
                case 2:
                    return os << "$tp";
                case 3:
                    return os << "$sp";
                case 22:
                    return os << "$fp";
            }
            if (4 <= id and id <= 11)
                return os << "$a" << id - 4;
            if (12 <= id and id <= 20)
                return os << "$t" << id - 12;
            if (23 <= id and id <= 31) {
                LOG_WARNING << "$s" << id - 23 << " may cause error";
                return os << "$s" << id - 23;
            }
            LOG_ERROR << "invalid register id: " << id;
            if (id < 0 or id > 31)
                abort();
            return os << "$r" << id;
        }
    }
};
constexpr size_t int_variant_index = 2;
constexpr size_t float_variant_index = 3;

using armval = std::variant<Reg, Value *, size_t, size_t>;

std::ostream &operator<<(std::ostream &os, const armval &val);

#ifndef SYSYC_USER_H
#define SYSYC_USER_H

#include "Value.h"

#include <cassert>
#include <memory>
#include <variant>
#include <vector>
class Instruction;
template <class>
inline constexpr bool always_false_v = false;

class User : public Value {
  public:
    // adapted from https://en.cppreference.com/w/cpp/memory/weak_ptr/~weak_ptr
    struct operand_t {
        std::variant<std::shared_ptr<Value>, std::weak_ptr<Value>> operand;
        operand_t(std::shared_ptr<Instruction> p);
        operand_t(std::shared_ptr<Value> p) : operand(p) {}
        operand_t(std::weak_ptr<Value> p) : operand(p) {}
        operand_t(std::nullptr_t _) : operand(nullptr) {}
        bool is_weak() const { return std::holds_alternative<std::weak_ptr<Value>>(operand); }
        Value *operator->() const {
            return std::visit(
                [](auto &p) {
                    using T = std::decay_t<decltype(p)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<Value>>)
                        return p.get();
                    else if constexpr (std::is_same_v<T, std::weak_ptr<Value>>)
                        return p.lock().get();
                    else
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                },
                operand);
        }
        Value *get() const {
            return std::visit(
                [&](auto &p) -> Value * {
                    using T = std::decay_t<decltype(p)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<Value>>)
                        return p.get();
                    else if constexpr (std::is_same_v<T, std::weak_ptr<Value>>) {
                        assert(p.lock());
                        return p.lock().get();
                    } else
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                },
                operand);
        }
        std::shared_ptr<Value> get_shared() const {
            return std::visit(
                [&](auto &p) -> std::shared_ptr<Value> {
                    using T = std::decay_t<decltype(p)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<Value>>)
                        return p;
                    else if constexpr (std::is_same_v<T, std::weak_ptr<Value>>) {
                        assert(p.lock());
                        return p.lock();
                    } else
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                },
                operand);
        }
    };

    User(Type *ty, const std::string &name = "", unsigned num_ops = 0);
    ~User() = default;

    std::vector<operand_t> &get_operands();

    // start from 0
    const operand_t &get_operand(unsigned i) const;

    // start from 0
    void set_operand(unsigned i, operand_t v);
    void add_operand(operand_t v);

    unsigned get_num_operand() const;

    void remove_use_of_ops();
    void remove_operands(int index1, int index2);

  private:
    std::vector<operand_t> operands_;  // operands of this value
    unsigned num_ops_;
};

#endif  // SYSYC_USER_H

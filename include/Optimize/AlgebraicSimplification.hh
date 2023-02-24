#ifndef __ALG_SIMP_HH__
#define __ALG_SIMP_HH__

#include "Pass.hh"
#include "utils.hh"

#include <unordered_map>
#include <unordered_set>
#include <variant>

// 代数化简
// bool is_constant(shared_ptr<Value>);
class AlgebraicSimplification : public Pass {
  public:
    AlgebraicSimplification(Module *m) : Pass(m) {}
    void run();

    struct ExpTreeNode {
        enum class AlgebraicOp { add, sub, mul, div, leaf } op;
        enum class ExpType { int_, float_ } type;
        ExpTreeNode(std::shared_ptr<Value> val_,
                    std::shared_ptr<ExpTreeNode> l_,
                    std::shared_ptr<ExpTreeNode> r_,
                    AlgebraicOp op_)
            : val(val_), l(l_), r(r_), op(op_), new_val(nullptr) {
            if (val->get_type()->is_integer_type())
                type = ExpType::int_;
            else
                type = ExpType::float_;
            // if (l)
            //     is_const = l->is_const && r->is_const;
            // else
            set_constant();
        }
        ExpTreeNode(std::shared_ptr<Instruction> instr_,
                    std::shared_ptr<ExpTreeNode> l_,
                    std::shared_ptr<ExpTreeNode> r_)
            : val(instr_), l(l_), r(r_), new_val(nullptr) {
            if (val->get_type()->is_integer_type())
                type = ExpType::int_;
            else
                type = ExpType::float_;
            set_op(instr_->get_instr_type());
            // if (l)
            //     is_const = l->is_const && r->is_const;
            // else
            set_constant();
        }
        void set_op(Instruction::OpID ins_op) {
            switch (ins_op) {
                case Instruction::OpID::add:
                case Instruction::OpID::fadd:
                    op = AlgebraicOp::add;
                    break;
                case Instruction::OpID::sub:
                case Instruction::OpID::fsub:
                    op = AlgebraicOp::sub;
                    break;
                case Instruction::OpID::mul:
                case Instruction::OpID::fmul:
                    op = AlgebraicOp::mul;
                    break;
                case Instruction::OpID::sdiv:
                case Instruction::OpID::fdiv:
                    op = AlgebraicOp::div;
                    break;
                default:
                    break;
            }
        }
        void set_constant() {
            if (val == nullptr) {
                is_const = false;
                return;
            }
            is_const = is_constant(val.get());
            if (is_const) {
                if (val->get_type()->is_integer_type()) {
                    const_val = std::static_pointer_cast<ConstantInt>(val)->get_value();
                } else {
                    const_val = std::static_pointer_cast<ConstantFP>(val)->get_value();
                }
            }
        }
        int get_const_int_val() { return std::get<int>(const_val); }
        float get_const_float_val() { return std::get<float>(const_val); }
        shared_ptr<Value> get_newest_val() { return new_val ? new_val : val; }
        void set_new_val(shared_ptr<Value> _nv) { new_val = _nv; }
        void set_const_val(int i) { const_val = i; }
        void set_const_val(float f) { const_val = f; }
        std::shared_ptr<Value> val;
        std::shared_ptr<Value> new_val;
        std::shared_ptr<ExpTreeNode> l;
        std::shared_ptr<ExpTreeNode> r;
        bool is_const = false;
        std::variant<int, float> const_val = 0;
        bool dealed = false;
        bool need_replace = false;
        std::list<shared_ptr<Instruction>> new_instrs;
        std::unordered_map<shared_ptr<Value>, int> terms;
        void add_term(shared_ptr<Value> v, int num) {
            if (type == ExpType::int_) {
                if (auto c = utils::get_const_int_val(v.get()); c.has_value()) {
                    if (get_const_int_val() != 0)
                        need_replace = true;
                    const_val = get_const_int_val() + c.value() * num;
                    return;
                }
            }
            if (terms.find(v) == terms.end())
                terms.insert({v, num});
            else {
                terms[v] += num;
                need_replace = true;
            }
        }
        void add_term(shared_ptr<ExpTreeNode> node, int n) {
            if (node->op == AlgebraicOp::leaf)
                add_term(node->val, n);
            else {
                for (auto t : node->terms) {
                    add_term(t.first, t.second * n);
                }
                if (node->get_const_int_val() != 0) {
                    set_const_val(get_const_int_val() + node->get_const_int_val() * n);
                    need_replace = true;
                }
            }
        }
    };
    void reassociation(shared_ptr<ExpTreeNode>);
    void add_sub_node(shared_ptr<ExpTreeNode>);
    void add_sub_node_new(shared_ptr<ExpTreeNode>);
    void mul_node(shared_ptr<ExpTreeNode>);
    void mul_node_new(shared_ptr<ExpTreeNode>);
    void div_node(shared_ptr<ExpTreeNode>);
    // 有一个操作数为常数的整数乘法的强度削弱（替换为移位和加减法）
    void int_mul_weaken(std::shared_ptr<Function> func);
    void eliminate_invalid_operation(std::shared_ptr<Function>);
    static bool is_constant(Value *v);
    bool is_alge_inst(Instruction *instr) {
        if (!instr->get_type()->is_integer_type())
            return false;

        return static_cast<IntegerType *>(instr->get_type())->get_num_bits() == 32 &&
               (instr->is_add() || instr->is_sub() || instr->is_mul());
    }
    void replace_instruction(std::shared_ptr<Function>);
    std::shared_ptr<ExpTreeNode> get_node(std::shared_ptr<Value>);
    void update_tree(std::shared_ptr<Instruction>);
    std::unordered_map<Value *, shared_ptr<ExpTreeNode>> value2node;
    std::unordered_set<shared_ptr<ExpTreeNode>> roots;
    std::shared_ptr<ExpTreeNode> virtual_non_const_node;
};

#endif
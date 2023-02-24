// 编写后端时发现我们处理undef values的逻辑与gcc和clang都不太一样
// 比如说 %op1 = phi [undef, bb1] [10, bb2]
// op1分到了 Reg0，那么控制流从bb1过来的时候什么都不会做，r0原本是什么就是什么，从bb2分支过来的话会mov 10给r0
// 根据 https://llvm.org/docs/LangRef.html#undefined-values，undef可以是任何数
// 然后llvm的简化phi指令的逻辑也是(cf simplifyPHINode) https://llvm.org/doxygen/InstructionSimplify_8cpp_source.html
// 对于所有的input operand（不考虑undef）如果全部相同就替换（当然，得考虑支配信息，不过如果是常数的话那就简单了）
// 这样基本上可以消掉大多数的undef value

#include "InstructionSimplify.hh"

#include "utils.hh"

void InstructionSimplify::run() {
    dom = std::make_unique<Dominators>(m_);
    dom->run();
    for (auto f : m_->get_functions())
        for (auto bb : f->get_basic_blocks()) {
            auto [phi_begin, phi_end] = bb->get_phis();
            for (auto it = phi_begin; it != phi_end; ++it) {
                if (auto new_val = simplify_phi(std::dynamic_pointer_cast<PhiInst>(*it))) {
                    LOG_DEBUG << "simplify " << (*it)->print();
                    (*it)->replace_all_use_with(new_val);
                    (*it)->get_use_list().clear();
                }
            }
        }
}

std::shared_ptr<Value> InstructionSimplify::simplify_phi(std::shared_ptr<PhiInst> phi) {
    std::shared_ptr<Value> common_val{};
    for (size_t i = 0; i < phi->get_num_operand(); i += 2) {
        auto op = phi->get_operand(i)->shared_from_this();
        if (not common_val)
            common_val = op;
        if (utils::is_const_int(common_val.get()) and utils::is_const_int(op.get()))
            if (utils::get_const_int_val(common_val.get()) == utils::get_const_int_val(op.get()))
                continue;
        if (utils::is_const_fp(common_val.get()) and utils::is_const_fp(op.get()))
            if (utils::get_const_float_val(common_val.get()) == utils::get_const_float_val(op.get()))
                continue;
        if (common_val != op)
            return nullptr;
    }
    if (phi->get_parent()->get_pre_basic_blocks().size() > phi->get_num_operand() / 2)
        return dominates(common_val, phi) ? common_val : nullptr;
    return common_val;
}
/// \brief does a dominates b?
bool InstructionSimplify::dominates(std::shared_ptr<Value> a, std::shared_ptr<Instruction> b) {
    if (std::dynamic_pointer_cast<Constant>(a))
        return true;
    if (std::dynamic_pointer_cast<Argument>(a))
        return true;
    return false;  // 别的懒得判断...
}

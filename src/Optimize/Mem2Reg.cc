#include "Mem2Reg.hh"

#include "Module.h"

#include <queue>

using std::dynamic_pointer_cast;
using std::pair;
using std::queue;
using std::set;
using std::static_pointer_cast;

bool is_global_var(Value *l_val) { return dynamic_cast<GlobalVariable *>(l_val) != nullptr; }
bool is_gep_inst(Value *l_val) { return dynamic_cast<GetElementPtrInst *>(l_val) != nullptr; }
bool is_phi_inst(Value *l_val) { return dynamic_cast<PhiInst *>(l_val) != nullptr; }

bool is_global_var(shared_ptr<Value> l_val) { return dynamic_pointer_cast<GlobalVariable>(l_val) != nullptr; }
bool is_gep_inst(shared_ptr<Value> l_val) { return dynamic_pointer_cast<GetElementPtrInst>(l_val) != nullptr; }
bool is_phi_inst(shared_ptr<Value> l_val) { return dynamic_pointer_cast<PhiInst>(l_val) != nullptr; }

// TODO: 对于以下代码，无法正确 Mem2Reg
// int main() {
//     int s;
//     return s;
// }
// 可能考虑添加 Undef 类型，或者在 alloca 后加入 store 0

int phi_count = 0;
set<Instruction *> all_add_phi;

void Mem2Reg::run() {
    dominators = std::make_unique<Dominators>(m_);
    dominators->run();

    all_add_phi.clear();

    // for (auto &func : m_->get_functions()) {
    //     dominators->print_idom(func.get());
    //     dominators->print_dom_frontier(func.get());
    // }

    for (auto func_share : m_->get_functions()) {
        auto func = func_share.get();
        if (func->get_basic_blocks().size() > 0) {
            generate_phi(func);
            rename(func->get_entry_block().get());
        }
        remove_alloca(func);
    }
    LOG(INFO) << "Mem2reg finish";
}

void Mem2Reg::generate_phi(Function *func) {
    // 1. 找出函数中的所有不是全局变量、不是数组操作的 store 指令，以及其所在的基本块
    // set<Value *> global_live_var;
    map<shared_ptr<Value>, set<BasicBlock *>> live_var_to_blocks;
    for (auto bb_share : func->get_basic_blocks()) {
        auto bb = bb_share.get();
        for (auto inst_share : bb->get_instructions()) {
            auto inst = inst_share.get();
            if (inst->is_store()) {
                auto store_inst = dynamic_pointer_cast<StoreInst>(inst_share);
                auto l_val = store_inst->get_lval();
                // auto r_val = store_inst->get_rval();

                // store i32 a, i32 *b
                // a : r_val, b : l_val

                if (not is_global_var(l_val) and not is_gep_inst(l_val) and not is_phi_inst(l_val))
                    live_var_to_blocks[l_val].insert(bb);
            }
        }
    }

    // 2. 插入 Phi 指令
    set<pair<BasicBlock *, shared_ptr<Value>>> all_bb_phi;
    for (auto [var, appeared_basicblock] : live_var_to_blocks) {
        queue<BasicBlock *> bb_queue;
        for (auto bb : appeared_basicblock)
            bb_queue.push(bb);

        while (not bb_queue.empty()) {
            auto bb = bb_queue.front();
            bb_queue.pop();

            for (auto bb_dominance_frontier_bb : dominators->get_dom_frontier(bb)) {
                if (all_bb_phi.find({bb_dominance_frontier_bb, var}) == all_bb_phi.end()) {
                    auto element_type = var->get_type()->get_pointer_element_type();
                    auto phi_inst_share = PhiInst::create_phi(element_type);
                    phi_inst_share->set_lval(var);
                    phi_inst_share->set_parent(bb_dominance_frontier_bb);
                    phi_inst_share->set_name("phi_" + std::to_string(phi_count++) + "_" + var->get_name());
                    bb_dominance_frontier_bb->add_instr_begin(phi_inst_share);
                    all_add_phi.insert(phi_inst_share.get());
                    bb_queue.push(bb_dominance_frontier_bb);
                    all_bb_phi.insert({bb_dominance_frontier_bb, var});
                }
            }
        }
    }
}

map<Value *, vector<shared_ptr<Value>>> var_to_val_stack;

void Mem2Reg::rename(BasicBlock *bb) {
    // 3. stack[l_val] 中添加 Phi 指令
    for (auto inst_share : bb->get_instructions())
        if (inst_share->is_phi()) {  // all_add_phi.find(inst_share.get()) != all_add_phi.end()
            // l_val 是 Phi 指令对应的指针
            auto l_val = static_pointer_cast<PhiInst>(inst_share)->get_lval().get();
            var_to_val_stack[l_val].push_back(inst_share);
        }

    vector<Instruction::self_iterator> to_delete;
    for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end(); it++) {
        auto inst_share = *it;
        // 4. 对于 load 指令，用 stack[l_val] 的顶部元素替换
        if (inst_share->is_load()) {
            auto l_val = static_pointer_cast<LoadInst>(inst_share)->get_lval().get();
            // %3 = load i32,ptr %i,;
            //l_val = %3;
            if (not is_global_var(l_val) and not is_gep_inst(l_val) and not is_phi_inst(l_val))
                if (var_to_val_stack.find(l_val) != var_to_val_stack.end()) {
                    auto phi_inst_share = var_to_val_stack[l_val].back();
                    inst_share->replace_all_use_with(phi_inst_share);
                    to_delete.push_back(it);
                }
        }
        // 5. 对于 store 指令，向 stack[l_val] 中加入 r_val
        if (inst_share->is_store()) {
            auto l_val = static_pointer_cast<StoreInst>(inst_share)->get_lval().get();
            auto r_val = static_pointer_cast<StoreInst>(inst_share)->get_rval();

            if (not is_global_var(l_val) and not is_gep_inst(l_val) and not is_phi_inst(l_val)) {
                var_to_val_stack[l_val].push_back(r_val);
                to_delete.push_back(it);
            }
        }
    }

    for (auto succ_bb : bb->get_succ_basic_blocks())
        for (auto inst_share : succ_bb->get_instructions()) {
            auto inst = inst_share.get();
            if (inst_share->is_phi() and all_add_phi.find(inst) != all_add_phi.end()) {
                auto phi_inst_share = static_pointer_cast<PhiInst>(inst_share);
                auto l_val = phi_inst_share->get_lval().get();
                if (var_to_val_stack.find(l_val) != var_to_val_stack.end() and var_to_val_stack[l_val].size() > 0) {
                    // 6. 确定 Phi 指令的参数
                    auto bb_share = static_pointer_cast<BasicBlock>(bb->shared_from_this());
                    auto back = var_to_val_stack[l_val].back();
                    if (dynamic_pointer_cast<Instruction>(back))
                        phi_inst_share->add_phi_pair_operand(std::weak_ptr(back), bb_share);
                    else
                        phi_inst_share->add_phi_pair_operand(back, bb_share);
                }
            }
        }

    // 递归 dom_tree_succ_blocks
    for (auto dom_succ_bb : dominators->get_dom_tree_succ_blocks(bb))
        rename(dom_succ_bb);

    // 7. 弹出 stack[l_val] 的顶部元素
    for (auto inst_share : bb->get_instructions()) {
        if (inst_share->is_store()) {
            auto l_val = static_pointer_cast<StoreInst>(inst_share)->get_lval().get();
            if (not is_global_var(l_val) and not is_gep_inst(l_val) and not is_phi_inst(l_val))
                var_to_val_stack[l_val].pop_back();
        }
        if (inst_share->is_phi()) {  // all_add_phi.find(inst_share.get()) != all_add_phi.end()
            auto l_val = static_pointer_cast<PhiInst>(inst_share)->get_lval().get();
            if (var_to_val_stack.find(l_val) != var_to_val_stack.end())  // var_to_val_stack[l_val].size() > 0
                var_to_val_stack[l_val].pop_back();
        }
    }

    // delete
    for (auto it : to_delete)
        bb->delete_instr(it);
}

void Mem2Reg::remove_alloca(Function *func) {
    for (auto bb : func->get_basic_blocks()) {
        vector<Instruction::self_iterator> to_delete;
        for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end(); it++) {
            auto inst_share = *it;
            if (inst_share->is_alloca()) {
                auto alloca_inst_share = static_pointer_cast<AllocaInst>(inst_share);
                auto element_type = alloca_inst_share->get_type()->get_pointer_element_type();
                if (element_type->is_integer_type() or element_type->is_float_type())
                    to_delete.push_back(it);
            }
        }
        for (auto it : to_delete)
            bb->delete_instr(it);
    }
}

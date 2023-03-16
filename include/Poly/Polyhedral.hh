#pragma once

#include "AliasAnalysis.hh"
#include "Dominators.hh"
#include "LoopInfo.hh"
#include "Pass.hh"
#include <stack>
#include "PolyBuilder.hh"
#include <isl/options.h>
#include <isl/typed_cpp.h>
#include <instruction.h>
isl::schedule_node optimize_band_node(isl::schedule_node n);

#include <isl/options.h>
#include <isl/typed_cpp.h>
#include <unordered_map>
class PolyStmt;
class PolyCFG;
class LoopExeOrder;
class PolyCFG{
    friend Polyhedral;
  public:
    PolyCFG(Module *m) : module(m){}
    BasicBlock* get_loop_before_blocks() { return this->loop_before; }
    BasicBlock* get_loop_exit_blocks() { return this->loop_exit; }
    void add_loop_before_block(BasicBlock* bb) { this->loop_before = bb; }
    void add_loop_exit_block(BasicBlock* bb) { this->loop_exit=bb; }
    void transform_cfg(){
        // LOG_DEBUG<<"tansform_cfg";
        auto main_func = *std::find_if(module->get_functions().begin(), module->get_functions().end(), [](auto f) { return f->get_name() == "main"; });
        auto cur_func = main_func.get();
        auto label_before_loop = this->get_loop_before_blocks(); //获取第一个loop_before_loop的前继
        auto label_before_loop_pre_list = label_before_loop->get_pre_basic_blocks_not_ref();//获取其父节点
        auto label_before_loop_pre = label_before_loop_pre_list.back();
        auto label_exit_loop = this->get_loop_exit_blocks();//获取loop_exit
        auto label_exit_loop_succ = label_exit_loop->get_succ_basic_blocks().back();//获取loop_after
        //根据label_exit_loop删除除开label_before_loop_pre的所有前驱块
        std::stack<BasicBlock *> delete_BB_stack;
        std::stack<BasicBlock *> delete_BB_stack_new;
        std::set<BasicBlock *> visit_BB;
        delete_BB_stack.push(label_exit_loop);
        while(!delete_BB_stack.empty()){
            BasicBlock* now_BB = delete_BB_stack.top();
            delete_BB_stack.pop();
            visit_BB.insert(now_BB);
            auto now_BB_pre_list = now_BB->get_pre_basic_blocks_not_ref();
            for(auto now_BB_pre:now_BB_pre_list){
                if(now_BB_pre==label_before_loop_pre||visit_BB.find(now_BB_pre)!=visit_BB.end()){
                    continue;
                }
                delete_BB_stack.push(now_BB_pre);
            }
            // LOG_DEBUG<<now_BB->print();
            cur_func->remove_not_bb(std::dynamic_pointer_cast<BasicBlock>(now_BB->shared_from_this()));
                // delete_BB_stack_new.push(now_BB);
                // cur_func->remove_unreachable_basic_block(std::dynamic_pointer_cast<BasicBlock>(now_BB->shared_from_this()));
        }
    }
  private:
    Module *module;
    BasicBlock* loop_before;
    BasicBlock* loop_exit;
};


class Polyhedral : public Pass {
    friend PolyStmt;
    friend PolyBuilder;
    friend PolyCFG;

  public:
    Polyhedral(Module *m) : Pass(m), loop_info(m), dom{m}, ir_inserter(m, this), poly_cfg(m){}
    void run() override;

    std::string get_stmt_name(Value *v) {
        auto find = val2name.find(v);
        if (find != val2name.end())
            return find->second;
        auto new_name = "STMT" + std::to_string(stmt_count++);
        val2name.insert({v, new_name});
        name2val.insert({new_name, v});
        return new_name;
    }

    std::string get_bound_name(Value *v) {
        auto find = val2name.find(v);
        if (find != val2name.end())
            return find->second;
        auto new_name = "N" + std::to_string(bound_count++);
        val2name.insert({v, new_name});
        name2val.insert({new_name, v});
        return new_name;
    }

    std::string get_iv_name(Value *v) {
        auto find = val2name.find(v);
        if (find != val2name.end())
            return find->second;
        auto new_name = "i" + std::to_string(bound_count++);
        val2name.insert({v, new_name});
        name2val.insert({new_name, v});
        return new_name;
    }

    std::string get_value_name(Value *v) {
        auto find = val2name.find(v);
        if (find != val2name.end())
            return find->second;
        auto new_name = "V" + std::to_string(bound_count++);
        val2name.insert({v, new_name});
        name2val.insert({new_name, v});
        return new_name;
    }

    std::string iv_range_to_string(Loop::InductionVar *iv) {
        std::string range_str;
        std::string initial_str;
        std::string final_str;
        if (auto c_i = iv->get_const_init_val(); c_i.has_value())
            initial_str = std::to_string(c_i.value());
        else
            initial_str = get_bound_name(iv->initial_val.get());
        if (auto c_f = iv->get_const_final_val(); c_f.has_value())
            final_str = std::to_string(c_f.value());
        else
            final_str = get_bound_name(iv->final_val.get());
        range_str += initial_str;
        range_str += " <= ";
        range_str += get_iv_name(iv->inst.get());
        range_str += (iv->predicate == (CmpOp)CmpOp::LT ? " < " : " <= ");
        range_str += final_str;
        return range_str;
    }

    void add_scheduled(Loop *l) {
        for (auto sub : l->get_sub_loops()) {
            add_scheduled(sub);
        }
        scheduled.insert(l);
    }
    bool is_valid_loop(Loop *);

    void build_loop_order(Loop *L,
                          std::vector<std::shared_ptr<Instruction>> &insts,
                          std::unordered_map<Loop *, std::shared_ptr<LoopExeOrder>> &loop2order) {
        loop2order.insert({L, std::make_shared<LoopExeOrder>(L, insts, loop_info, dom)});
        for (auto sub : L->get_sub_loops()) {
            build_loop_order(sub, insts, loop2order);
        }
    }

    Value *get_value(std::string name) {
        auto val = name2val.find(name);
        if (val != name2val.end()) {
            return val->second;
        }
        return nullptr;
    }

  private:
    LoopInfo loop_info;
    Dominators dom;
    PolyBuilder ir_inserter;
    PolyCFG poly_cfg;
    std::unordered_map<Value *, std::string> val2name;
    std::unordered_map<std::string, Value *> name2val;

    int stmt_count = 0;
    int bound_count = 0;

    isl::union_set domain;
    std::unordered_set<Loop *> scheduled;
};

class PolyStmt {
    friend PolyBuilder;
    friend Polyhedral;

  public:
    PolyStmt(std::shared_ptr<Instruction> inst, Loop *loop, Polyhedral &p, std::vector<Loop::InductionVar *> &ind_vars)
        : inst_(inst), loop_(loop), poly(p), idxs(ind_vars) {
        writes.insert(inst);
        std::unordered_set<Value *> visited;
        find_read(inst.get(), visited);
        this->name = poly.get_stmt_name(inst.get());
    }
    std::string get_name() { return name; }
    std::string get_idx_string() {
        std::string idx_string;
        idx_string += "[";
        if (!idxs.empty())
            idx_string += poly.get_iv_name(idxs[0]->inst.get());
        for (int i = 1; i < idxs.size(); i++) {
            idx_string += ",";
            idx_string += poly.get_iv_name(idxs[i]->inst.get());
        }
        idx_string += "]";
        return idx_string;
    }

    // 返回形如S[i,j,k]的字符串
    std::string get_name_idx() { return get_name() + get_idx_string(); }

    std::string get_domain_str() {
        std::string domain_str;
        std::unordered_set<std::string> bound_names;
        domain_str += "[";
        // if (!idxs.empty()) {
        //     auto bd_val = idxs[0]->final_val.get();
        //     auto bd_name = poly.get_bound_name(bd_val);
        //     if (!isa<Constant>(bd_val)) {
        //         domain_str += bd_name;
        //         bound_names.insert(bd_name);
        //     }
        // }
        for (int i = 0; i < idxs.size(); i++) {
            auto bd_val = idxs[i]->final_val.get();
            auto bd_name = poly.get_bound_name(bd_val);
            if (!isa<Constant>(bd_val) && bound_names.find(bd_name) == bound_names.end()) {
                if (!bound_names.empty())
                    domain_str += ",";
                domain_str += bd_name;
                bound_names.insert(bd_name);
            }
        }
        domain_str += "] -> { ";
        domain_str += get_name_idx();
        domain_str += " : ";
        domain_str += poly.iv_range_to_string(idxs[0]);
        for (int i = 1; i < idxs.size(); i++) {
            domain_str += " and ";
            domain_str += poly.iv_range_to_string(idxs[i]);
        }
        domain_str += " }";
        return domain_str;
    }

    isl::union_set get_domain(isl::ctx ctx) { return isl::union_set(ctx, get_domain_str()); }
    std::unordered_set<std::shared_ptr<Instruction>> &get_reads() { return reads; }
    std::unordered_set<std::shared_ptr<Instruction>> &get_writes() { return writes; }

  private:
    void find_read(Instruction *inst, std::unordered_set<Value *> &visited) {
        if (visited.find(inst) != visited.end())
            return;
        if (inst->is_load()){
            LOG_DEBUG<<inst->print();
            reads.insert(inst->get_shared_ptr());
        }
        visited.insert(inst);
        for (int i = 0; i < inst->get_num_operand(); i++) {
            auto op = inst->get_operand(i).get();
            auto op_inst = dynamic_cast<Instruction *>(op);
            if (op_inst)
                find_read(op_inst, visited);
        }
    }
    std::shared_ptr<Instruction> inst_;
    Loop *loop_;
    Polyhedral &poly;
    std::vector<Loop::InductionVar *> idxs;
    std::string name;
    std::unordered_set<std::shared_ptr<Instruction>> writes;
    std::unordered_set<std::shared_ptr<Instruction>> reads;
};

class LoopExeOrder {
  public:
    LoopExeOrder(Loop *l, std::vector<std::shared_ptr<Instruction>> &insts, LoopInfo &LI, Dominators &dom)
        : loop(l), LI_(LI), dom_(dom) {
        bb_set_t sub_blocks;
        std::list<Instruction *> sort_inst;
        for (auto sub : l->get_sub_loops()) {
            sub_list.push_back(sub);
            // auto sub_guard = sub->get_preheader()->get_pre_basic_blocks().back();
            insert_and_sort(get_loop_proxy(sub), sort_inst);
            // sub_blocks.insert(sub->get_blocks().begin(), sub->get_blocks().end());
        }
        for (auto inst : insts) {
            auto inst_loop = LI.get_inner_loop(inst->get_parent());
            if (inst_loop == l) {
                insert_and_sort(inst.get(), sort_inst);
            }
        }
        int idx = 0;
        for (auto inst : sort_inst) {
            inst_idx.insert({inst, idx});
            LOG_DEBUG<<(*inst).print()<<" "<<idx;
            idx++;
        }
    }

    // 如果inst1在inst2之前返回true
    bool inst_before(Instruction *inst1, Instruction *inst2) {
        auto bb1 = inst1->get_parent();
        auto bb2 = inst2->get_parent();
        if (bb1 != bb2) {
            if (dom_.is_dominator(bb2, bb1))
                return true;
            return false;
        } else {
            for (auto inst : *bb1) {
                if (inst.get() == inst1)
                    return true;
                else if (inst.get() == inst2)
                    return false;
            }
        }
    }

    void insert_and_sort(Instruction *I, std::list<Instruction *> &sort_inst) {
        for (auto itr = sort_inst.begin();; itr++) {
            if (itr == sort_inst.end()) {
                sort_inst.push_back(I);
                break;
            }
            if (inst_before(I, *itr)) {
                sort_inst.insert(itr, I);
                break;
            }
        }
    }

    Instruction *get_loop_proxy(Loop *l) {
        auto guard = l->get_preheader()->get_pre_basic_blocks().back();
        return (*guard->begin()).get();
    }

    int get_idx(Instruction *I) { return inst_idx.at(I); }

    int get_idx(Loop *L) { return inst_idx.at(get_loop_proxy(L)); }

  private:
    Loop *loop;
    std::vector<Loop *> sub_list;
    bb_set_t before_all_subs;
    std::vector<bb_set_t> after_sub;
    std::unordered_map<Loop *, int> sub_idx;
    std::unordered_map<Instruction *, int> inst_idx;
    LoopInfo &LI_;
    Dominators &dom_;
};

class PolyMemAccess {
  public:
    PolyMemAccess(Value *ptr) : addr(ptr) {
        auto geps = addr.get_gep_insts();
        for (int i = 0; i < geps.size(); i++) {
            auto gep = geps[i];
        
            if (gep->get_num_operand() == 3) {
                idxs.push_back(gep->get_operand(2).get());
            } else if (gep->get_num_operand() == 2 && i == 0) {
                idxs.push_back(gep->get_operand(1).get());
            } else {
                LOG_DEBUG<<(*gep).print();
                valid = false;
                return;
            }
        }
    }
    bool valid = true;
    std::vector<Value *> &get_idxs() { return idxs; }
    std::string get_str(std::vector<Loop::InductionVar *> &inds, Polyhedral &poly) {
        std::string result;
        result += poly.get_value_name(addr.get_base());
        result += "[";
        for (auto idx : idxs) {
            LOG_DEBUG<<((*idx).print());
            bool success1 = false;
            auto ins = dynamic_cast<Instruction *>(idx);
            if((*ins).is_add()||(*ins).is_sub()){
                auto op_0 = ins->get_operand(0).get();
                auto op_1 = ins->get_operand(1).get();
                for(auto ind:inds){
                    if (op_0 == (ind->inst.get())) {
                        result += poly.get_iv_name(op_0);
                        success1 = true;
                        if((*ins).is_add()){
                            result+='+';
                        }
                        if((*ins).is_sub()){
                            result+='-';
                        }
                    }
                    if(op_1 == (ind->inst.get())){
                        result += poly.get_iv_name(op_1);
                        success1 = true;
                        if((*ins).is_add()){
                            result+='+';
                        }
                        if((*ins).is_sub()){
                            result+='-';
                        }
                    }
                }
            }else{
                bool success = false;
                //直接是常量
                if (auto c_i = utils::get_const_int_val(idx); c_i.has_value()) {
                    result += std::to_string(c_i.value());
                    success = true;
                } else {
                    for (auto ind : inds) {
                        //idx是对应的归纳变量指令
                        if (idx == ind->inst.get()) {
                            result += poly.get_iv_name(idx);
                            success = true;
                            break;
                        } else {
                            ConstantInt *c;
                            if (match(idx, m_Add(m_specific(ind->inst.get()), m_constantInt(c)))) {
                                result += std::to_string(c->get_value());
                                result += "+";
                                result += poly.get_iv_name(ind->inst.get());
                                success = true;
                                break;
                            } else if (match(idx, m_Sub(m_specific(ind->inst.get()), m_constantInt(c)))) {
                                result += poly.get_iv_name(ind->inst.get());
                                if (c->get_value() > 0)
                                    result += "-";
                                result += std::to_string(c->get_value());
                                success = true;
                                break;
                            }
                        }
                    }
                }
                if (success) {
                    result += ",";
                } else {
                    return "";
                }
            }
        }
        result.pop_back();
        result += "]";
        return result;
    }

    std::vector<Value *> idxs;
    MemAddress addr;
};
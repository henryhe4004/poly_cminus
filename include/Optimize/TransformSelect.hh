#pragma once
#include "Instruction.h"
#include "Module.h"
#include "Pass.hh"
#include "utils.hh"
#include "Function.h"
#include "BasicBlock.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class TransformSelect : public Pass {
  public:
    TransformSelect(Module *m) : Pass(m), module(m){}
    void run(){
      LOG_DEBUG<<"TransferSelect begin";
      LOG_DEBUG<<module->print();
       auto main_func = *std::find_if(
        module->get_functions().begin(), module->get_functions().end(), [](auto f) { return f->get_name() == "main"; });
      // LOG_DEBUG<<module->print();
    // std::list<std::shared_ptr<Function>> main_func_list = module->get_functions();
    // std::shared_ptr<Function> main_func = main_func_list.front();
    auto cur_func = main_func.get();
    auto entry = cur_func->get_entry_block(); //需要提前声明
    std::list<std::shared_ptr<BasicBlock>> basic_list = cur_func->get_basic_blocks();
    for(auto bb_shared :basic_list){
        LOG_DEBUG<<bb_shared.get()->print();
        bool move_instruction = false;
        auto bb = bb_shared.get();
        auto bb_succ_list = bb->get_succ_basic_blocks_not_ref();
        // Instruction *lhs = new Instruction();
        // Instruction *rhs = new Instruction();

        auto inst_list = bb->get_instructions();
        inst_list_type::iterator iter = inst_list.begin();
        for(;iter!=inst_list.end();iter++){
            auto inst = *iter;
            if(inst.get()->is_select()){
              auto insert_bb_true = BasicBlock::create(module, bb->get_name()+"_true", cur_func);  
              auto insert_bb_false = BasicBlock::create(module,bb->get_name()+"_false",cur_func);
              auto insert_bb_end = BasicBlock::create(module,bb->get_name()+"_out",cur_func);
              LOG_DEBUG<<inst.get()->print();
                move_instruction = true;
                for(auto bb_succ : bb_succ_list){
                    bb_succ->remove_pre_basic_block(bb);
                    bb_succ->add_pre_basic_block(insert_bb_end.get());
                    bb->remove_succ_basic_block(bb_succ);
                    insert_bb_end.get()->add_succ_basic_block(bb_succ);
                }
                bb->add_succ_basic_block(insert_bb_true.get());
                bb->add_succ_basic_block(insert_bb_false.get());
                insert_bb_true.get()->add_pre_basic_block(bb);
                insert_bb_false.get()->add_pre_basic_block(bb);
                insert_bb_true.get()->add_succ_basic_block(insert_bb_end.get());
                insert_bb_false.get()->add_succ_basic_block(insert_bb_end.get());
                insert_bb_end.get()->add_pre_basic_block(insert_bb_true.get());
                insert_bb_end.get()->add_pre_basic_block(insert_bb_false.get());
                auto lhs = (inst.get())->get_operand(1).get();
                auto rhs = (inst.get())->get_operand(2).get();
                auto  test_bit = (inst.get())->get_operand(0).get();
                // 在调用max min select语句都是倒数3条
                int inst_num = bb->get_num_inst();
                BranchInst::create_cond_br(test_bit->shared_from_this(),insert_bb_true,insert_bb_false,bb);
                // inst_list_type::iterator iter = entry.get()->get_instructions();
                auto entry_br = entry.get()->get_terminator();
                entry.get()->delete_instr(entry_br);
                auto inst_alloca = AllocaInst::create_alloca(Type::get_int32_type(module),entry.get());
                entry.get()->add_instruction(entry_br);
                StoreInst::create_store(lhs->shared_from_this(),inst_alloca,insert_bb_true.get());
                BranchInst::create_br(insert_bb_end,insert_bb_true.get());
                StoreInst::create_store(rhs->shared_from_this(),inst_alloca,insert_bb_false.get());
                BranchInst::create_br(insert_bb_end,insert_bb_false.get());
                iter++;
                bool cmp_before = false;
                std::shared_ptr<CmpInst> inst_temp_cmp = nullptr;
                for(;iter!=inst_list.end();iter++){
                    auto inst_after = *(iter);
                    bb->delete_instr(inst_after);

                    if(inst_after.get()->is_store()){
                        StoreInst::create_store(inst_alloca,(inst_after.get()->get_operand(1).get())->shared_from_this(),insert_bb_end.get());
                    }else if(inst_after.get()->is_cmp()){
                        inst_temp_cmp = CmpInst::create_cmp(static_cast<CmpInst*>(inst_after.get())->get_cmp_op(),(inst_after.get()->get_operand(0).get())->shared_from_this(),inst_alloca,insert_bb_end.get());
                        cmp_before = true;
                    }else if(inst_after.get()->is_br()){
                        //应该还判断一下条件
                        if(static_cast<BranchInst*>(inst_after.get())->is_cond_br()&& cmp_before == true){
                          BranchInst::create_cond_br(inst_temp_cmp,std::dynamic_pointer_cast<BasicBlock>(inst_after.get()->get_operand(1).get()->shared_from_this()),std::dynamic_pointer_cast<BasicBlock>(inst_after.get()->get_operand(2).get()->shared_from_this()),insert_bb_end.get());
                        }else{
                          insert_bb_end.get()->add_instruction(inst_after);
                        }                    
                    }
                }
                // auto inst_icmp =  *(++iter);
                // auto inst_br = *(++iter);
                // bb->delete_instr(inst_icmp);
                // bb->delete_instr(inst_br);
                // CmpInst::create_cmp(static_cast<CmpInst*>(inst_icmp.get())->get_cmp_op(),(inst_icmp.get()->get_operand(0).get())->shared_from_this(),inst_alloca,insert_bb_end.get());
                // insert_bb_end.get()->add_instruction(inst_br);
                bb->delete_instr(inst);
                break;
            }
        }
    }
    }
    
  private:
    Module *module;
};

#ifndef __LOOPINVMOTION_HH__
#define __LOOPINVMOTION_HH__

#include "AliasAnalysis.hh"
#include "Dominators.hh"
#include "FuncInfo.hh"
#include "LoopSearch.hh"
#include "Pass.hh"

#include <vector>

class LoopInvMotion : public Pass {
  public:
    LoopInvMotion(Module *m) : Pass(m), _loops(m), _dom(m), func_info(m) {}
    void run();

  private:
    loop_search _loops;
    Dominators _dom;
    FuncInfo func_info;
    std::vector<std::shared_ptr<bb_set_t>> get_sub_loops(std::shared_ptr<bb_set_t>);
    void get_all_doms(BasicBlock *, std::unordered_set<BasicBlock *> &);
    bool is_store_movable(std::shared_ptr<StoreInst> I, bb_set_t *loop) {
        auto ptr = I->get_operand(1).get();
        // auto ptr_gep
        AliasAnalysis AA;
        for (auto bb : *loop) {
            for (auto inst : *bb) {
                if (inst->is_store() && inst != I) {
                    if (inst->get_operand(1)->get_type() != ptr->get_type())
                        continue;
                    if (AA.alias(ptr, inst->get_operand(1).get()) != AliasResult::NoAlias)
                        return false;
                } else if (inst->is_load()) {
                    if (inst->get_operand(0)->get_type() != ptr->get_type())
                        continue;
                    if (AA.alias(ptr, inst->get_operand(0).get()) != AliasResult::NoAlias) {
                        if (I->get_parent() == inst->get_parent()) {
                            for (auto itr = inst->get_iterator(); itr != inst->get_parent()->end(); itr++) {
                                if (*itr == I)
                                    return false;
                            }
                        } else if (!_dom.is_dominator(inst->get_parent(), I->get_parent()))
                            return false;
                    }
                }
            }
        }
        return true;
    }
    int counter = 0;
};

#endif  // #ifndef __LOOPINVMOTION_HH__
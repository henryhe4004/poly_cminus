#ifndef OPTIMIZE_DOMINATORS_HH
#define OPTIMIZE_DOMINATORS_HH

#include "Pass.hh"

#include <map>

using std::cout;
using std::endl;
using std::map;
using std::set;

class Dominators : public Pass {
  public:
    Dominators(Module *m) : Pass(m) {}

    void run();
    void log() {
        for (auto func : m_->get_functions()) {
            if (func->is_declaration())
                continue;
            LOG_INFO << "PostDom: infos about " << func->get_name() << "\n";
            print_idom(func.get());
            print_dom_frontier(func.get());
        }
    }

    void print_idom(Function *func);
    void print_dom_frontier(Function *func);

    BasicBlock *get_idom(BasicBlock *block) { return idom_[block]; }
    set<BasicBlock *> &get_dom_frontier(BasicBlock *block) { return dom_frontier_[block]; }
    set<BasicBlock *> &get_dom_tree_succ_blocks(BasicBlock *block) { return dom_tree_succ_blocks_[block]; }

    bool is_dominator(BasicBlock *succ, BasicBlock *prev);

  private:
    // map<BasicBlock *, set<BasicBlock *>> doms_;
    map<BasicBlock *, BasicBlock *> idom_;
    map<BasicBlock *, set<BasicBlock *>> dom_frontier_;
    map<BasicBlock *, set<BasicBlock *>> dom_tree_succ_blocks_;

    void generate_idom(Function *func);
    void generate_dom_frontier(Function *func);
    void generate_dom_tree_succ_blocks(Function *func);

    BasicBlock *intersect(BasicBlock *b1, BasicBlock *b2);
};

#endif

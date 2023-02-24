#pragma once

#include "Dominators.hh"

#include <algorithm>

class PostDominators : public Pass {
  public:
    PostDominators(Module *m) : Pass(m) {}
    void run() {
        // doms_.clear();
        idom_.clear();
        dom_frontier_.clear();
        dom_tree_succ_blocks_.clear();

        for (auto func : m_->get_functions())
            if (func->get_basic_blocks().size() > 1) {
                auto function = func.get();
                visit_function(function);
                generate_idom(function);
                generate_dom_frontier(function);
                generate_dom_tree_succ_blocks(function);
            }
    }

    void log() {
        for (auto func : m_->get_functions()) {
            if (func->is_declaration())
                continue;
            LOG_INFO << "PostDom: infos about " << func->get_name() << "\n";
            print_idom(func.get());
            print_dom_frontier(func.get());
        }
    }

    void print_idom(Function *func) {
        if (func->get_basic_blocks().size() == 0)
            return;
        printf("basic block\tidom\n");
        for (auto bb_share : func->get_basic_blocks()) {
            auto bb = bb_share.get();
            // if null, print null
            printf("%-20s", bb->get_name().c_str());
            if (idom_[bb] == nullptr)
                printf("null");
            else
                printf("%s", idom_[bb]->get_name().c_str());
            printf("\n");
        }
        printf("\n");
    }
    void print_dom_frontier(Function *func) {
        if (func->get_basic_blocks().size() == 0)
            return;
        printf("basic block\tdom frontier\n");
        for (auto bb_share : func->get_basic_blocks()) {
            auto bb = bb_share.get();
            printf("%-20s[", bb->get_name().c_str());
            for (auto succ_bb : dom_frontier_[bb])
                printf("%s,", succ_bb->get_name().c_str());
            printf("]\n");
        }
        printf("\n");
    }

    BasicBlock *get_idom(BasicBlock *block) { return idom_[block]; }
    set<BasicBlock *> &get_dom_frontier(BasicBlock *block) { return dom_frontier_[block]; }
    set<BasicBlock *> &get_dom_tree_succ_blocks(BasicBlock *block) { return dom_tree_succ_blocks_[block]; }

    bool is_dominator(BasicBlock *succ, BasicBlock *prev) {
        // Is succ a dominator of prev?
        // 到达 succ 之前是否必然到达 prev（严格）
        if (idom_[succ] == succ)  // 如果 succ 是初始块
            return false;
        if (idom_[succ] == prev)
            return true;
        return is_dominator(idom_[succ], prev);
    }

  private:
    // map<BasicBlock *, set<BasicBlock *>> doms_;
    map<BasicBlock *, BasicBlock *> idom_;
    map<BasicBlock *, set<BasicBlock *>> dom_frontier_;
    map<BasicBlock *, set<BasicBlock *>> dom_tree_succ_blocks_;

    void generate_idom(Function *func) {
        for (auto block_share : func->get_basic_blocks())
            idom_[block_share.get()] = nullptr;
        auto root = func->get_term_block().get();
        idom_[root] = root;

        bool changed = true;
        while (changed) {
            changed = false;

            for (auto block : visit_order) {
                if (block == root)
                    continue;

                // 找到 block 的前驱块中已经记录 idom 的块 pre_block
                BasicBlock *succ_block = nullptr;
                for (auto temp_succ_block : block->get_succ_basic_blocks())
                    if (idom_[temp_succ_block] != nullptr) {
                        succ_block = temp_succ_block;
                        break;
                    }
                assert(succ_block != nullptr);

                // 如果有多个前驱，new_idom 为所有前驱的最近公共祖先
                auto new_idom = succ_block;
                for (auto temp_succ_block : block->get_succ_basic_blocks())
                    if (idom_[temp_succ_block] != nullptr and temp_succ_block != succ_block) {
                        new_idom = intersect(new_idom, temp_succ_block);
                    }

                if (idom_[block] != new_idom) {
                    idom_[block] = new_idom;
                    changed = true;
                }
            }
        }
    }
    void generate_dom_frontier(Function *func) {
        for (auto bb_share : func->get_basic_blocks()) {
            auto bb = bb_share.get();
            if (bb->get_succ_basic_blocks().size() >= 2)
                for (auto pre_bb : bb->get_succ_basic_blocks()) {
                    for (auto temp_bb = pre_bb; temp_bb != idom_[bb]; temp_bb = idom_[temp_bb])
                        dom_frontier_[temp_bb].insert(bb);
                }
        }
    }
    void generate_dom_tree_succ_blocks(Function *func) {
        for (auto bb_share : func->get_basic_blocks()) {
            auto bb = bb_share.get();
            auto idom = idom_[bb];
            if (idom != bb)
                dom_tree_succ_blocks_[idom].insert(bb);
        }
    }

    BasicBlock *intersect(BasicBlock *b1, BasicBlock *b2) {
        while (b1 != b2) {
            if (visit_index[b1] < visit_index[b2])
                b1 = idom_[b1];
            else
                b2 = idom_[b2];
        }
        assert(b1 != nullptr);
        return b1;
    }

    vector<BasicBlock *> visit_order;
    map<BasicBlock *, bool> visited;
    map<BasicBlock *, int> visit_index;

    void visit_block(BasicBlock *block) {
        visited[block] = true;
        for (auto succ : block->get_pre_basic_blocks())
            if (!visited[succ])
                visit_block(succ);

        visit_index[block] = visit_order.size();
        visit_order.push_back(block);
    }

    void visit_function(Function *func) {
        visit_order.clear();
        visited.clear();
        visit_index.clear();

        for (auto &bb : func->get_basic_blocks())
            visited[bb.get()] = false;

        visit_block(func->get_term_block().get());

        // delete unreachable blocks
        // vector<BasicBlock *> to_delete;
        vector<shared_ptr<BasicBlock>> to_delete;
        for (auto &bb_share : func->get_basic_blocks())
            if (!visited[bb_share.get()])
                to_delete.push_back(bb_share);

        for (auto bb : to_delete)
            func->remove_unreachable_basic_block(bb);

        assert(visit_order.size() == func->get_basic_blocks().size());
        reverse(visit_order.begin(), visit_order.end());
    }
};
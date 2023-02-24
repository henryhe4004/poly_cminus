#pragma once
#include "Pass.hh"

#include <queue>
#include <unordered_set>

class SortBB : public Pass {
  public:
    SortBB(Module *m) : Pass(m) {}
    void run() {
        for (auto func : m_->get_functions()) {
            if (func->is_declaration())
                continue;
            std::list<std::shared_ptr<BasicBlock>> sorted;
            std::queue<std::shared_ptr<BasicBlock>> bb_queue;
            std::unordered_set<BasicBlock *> visited;
            bb_queue.push(func->get_entry_block());
            while (!bb_queue.empty()) {
                auto bb = bb_queue.front();
                bb_queue.pop();
                // visited.insert(old_bb);
                for (auto succ : bb->get_succ_basic_blocks()) {
                    auto succ_shared = std::static_pointer_cast<BasicBlock>(succ->shared_from_this());
                    if (visited.find(succ) == visited.end()) {
                        bb_queue.push(succ_shared);
                        visited.insert(succ);
                    }
                }
                sorted.push_back(bb);
            }
            func->get_basic_blocks().assign(sorted.begin(), sorted.end());
        }
    }
};
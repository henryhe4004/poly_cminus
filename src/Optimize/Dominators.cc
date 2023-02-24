#include "Dominators.hh"

#include <algorithm>

using std::reverse;

vector<BasicBlock *> visit_order;
map<BasicBlock *, bool> visited;
map<BasicBlock *, int> visit_index;

// 后序遍历
void visit_block(BasicBlock *block) {
    visited[block] = true;
    for (auto succ : block->get_succ_basic_blocks())
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

    visit_block(func->get_entry_block().get());

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

void Dominators::run() {
    // doms_.clear();
    idom_.clear();
    dom_frontier_.clear();
    dom_tree_succ_blocks_.clear();

    for (auto function_share : m_->get_functions())
        if (function_share->get_basic_blocks().size() > 1) {
            auto function = function_share.get();
            visit_function(function);
            generate_idom(function);
            generate_dom_frontier(function);
            generate_dom_tree_succ_blocks(function);
        }
}

BasicBlock *Dominators::intersect(BasicBlock *b1, BasicBlock *b2) {
    while (b1 != b2) {
        if (visit_index[b1] < visit_index[b2])
            b1 = idom_[b1];
        else
            b2 = idom_[b2];
    }
    assert(b1 != nullptr);
    return b1;
}

// int main() {
//     int n = 10;
//     int i = 0;
//     int sum = 0;
//     while (i < n) {
//         sum = sum + i;
//         i = i + 1;
//     }
//     return sum;
// }

// label_entry
// label_beforeloop0 -> label_out4
// label_preheader1
// label_body2
// label_latch3 -> label_body2
// label_out4
// label_return

// block                idom（经过 block 前必然到达的最近的基本块）     dom frontier
// label_entry          label_entry
// label_return         label_out4
// label_beforeloop0    label_entry
// label_preheader1     label_beforeloop0                       label_out4
// label_body2          label_preheader1                        label_body2, label_out4
// label_latch3         label_body2                             label_body2, label_out4
// label_out4           label_beforeloop0

void Dominators::generate_idom(Function *func) {
    for (auto block_share : func->get_basic_blocks())
        idom_[block_share.get()] = nullptr;
    auto root = func->get_entry_block().get();
    idom_[root] = root;

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto block : visit_order) {
            if (block == root)
                continue;

            // 找到 block 的前驱块中已经记录 idom 的块 pre_block
            BasicBlock *pre_block = nullptr;
            for (auto temp_pre_block : block->get_pre_basic_blocks())
                if (idom_[temp_pre_block] != nullptr) {
                    pre_block = temp_pre_block;
                    break;
                }
            assert(pre_block != nullptr);

            // 如果有多个前驱，new_idom 为所有前驱的最近公共祖先
            auto new_idom = pre_block;
            for (auto temp_pre_block : block->get_pre_basic_blocks())
                if (idom_[temp_pre_block] != nullptr and temp_pre_block != pre_block) {
                    new_idom = intersect(new_idom, temp_pre_block);
                }

            if (idom_[block] != new_idom) {
                idom_[block] = new_idom;
                changed = true;
            }
        }
    }
}

// 所有后继支配块
void Dominators::generate_dom_frontier(Function *func) {
    for (auto bb_share : func->get_basic_blocks()) {
        auto bb = bb_share.get();
        if (bb->get_pre_basic_blocks().size() >= 2)
            for (auto pre_bb : bb->get_pre_basic_blocks()) {
                for (auto temp_bb = pre_bb; temp_bb != idom_[bb]; temp_bb = idom_[temp_bb])
                    dom_frontier_[temp_bb].insert(bb);
            }
    }
}

void Dominators::generate_dom_tree_succ_blocks(Function *func) {
    for (auto bb_share : func->get_basic_blocks()) {
        auto bb = bb_share.get();
        auto idom = idom_[bb];
        if (idom != bb)
            dom_tree_succ_blocks_[idom].insert(bb);
    }
}

void Dominators::print_idom(Function *func) {
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

void Dominators::print_dom_frontier(Function *func) {
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

bool Dominators::is_dominator(BasicBlock *succ, BasicBlock *prev) {
    // Is succ a dominator of prev?
    // 到达 succ 之前是否必然到达 prev（严格）
    if (idom_[succ] == succ)  // 如果 succ 是初始块
        return false;
    if (idom_[succ] == prev)
        return true;
    return is_dominator(idom_[succ], prev);
}

#include "LoopSearch.hh"

#include "logging.hpp"

#include <fstream>
#include <unordered_set>

// 先扫描函数，生成 bb 对应的 cfgnode，接着根据 bb 的关系生成 cfg_node 之间的关系
void loop_search::build_cfg(std::shared_ptr<Function> func, std::unordered_set<cfg_node_pointer> &result) {
    std::unordered_map<BasicBlock *, std::shared_ptr<cfg_node>> bb2cfg_node{};
    for (auto bb : func->get_basic_blocks()) {
        auto node = std::make_shared<cfg_node>(bb.get(), -1, -1, false);
        bb2cfg_node.insert({bb.get(), node});

        result.insert(node);
    }

    for (auto bb : func->get_basic_blocks()) {
        auto node = bb2cfg_node[bb.get()];
        for (auto next : bb->get_succ_basic_blocks()) {
            node->next.insert(bb2cfg_node[next].get());
        }
        for (auto pre : bb->get_pre_basic_blocks()) {
            node->pre.insert(bb2cfg_node[pre].get());
        }
    }
}

// 找出图中所有的强联通分量，但是不会找出内层的 scc
bool loop_search::scc(cfg_node_pointer_set &nodes, std::unordered_set<std::shared_ptr<cfg_node_pointer_set>> &result) {
    index_count = 0;
    stack.clear();
    for (auto node : nodes) {
        if (node->dfn == -1)
            traverse(node, result);
    }
    return result.size();
}

// 找出 now 所在连通块的强联通分量
// tarjan 算法，https://oi-wiki.org/graph/scc/
// result 中存储了找到的所有强联通分量
void loop_search::traverse(cfg_node_pointer now, std::unordered_set<std::shared_ptr<cfg_node_pointer_set>> &result) {
    now->low = now->dfn = index_count++;
    stack.push_back(now);
    now->in_stack = true;

    for (auto next_raw : now->next) {
        auto next = next_raw->shared_from_this();
        if (next->dfn == -1) {
            traverse(next, result);
            now->low = std::min(next->low, now->low);
        } else if (next->in_stack) {
            now->low = std::min(next->dfn, now->low);
        }
    }

    if (now->dfn == now->low) {
        auto set = std::make_shared<cfg_node_pointer_set>();
        cfg_node_pointer tmp = nullptr;
        do {
            tmp = stack.back();
            tmp->in_stack = false;
            set->insert(tmp);
            stack.pop_back();
        } while (tmp != now);

        // TODO 检查找到的 scc 是否合法
        if (set->size() > 1)
            result.insert(set);
        else if (set->size() == 1) {
            // 将自环加入 scc
            auto node = *set->begin();
            for (auto next : node->next)
                if (next->shared_from_this() == node) {
                    result.insert(set);
                    break;
                }
        }
    }
}

cfg_node_pointer loop_search::find_loop_base(std::shared_ptr<cfg_node_pointer_set> set,
                                             cfg_node_pointer_set &reserved) {
    cfg_node_pointer base{nullptr};
    // 如果一个节点的前驱不在 scc（循环） 中，那么这个节点就是入口
    for (auto now : *set) {
        for (auto pre : now->pre) {
            if (set->find(pre->shared_from_this()) == set->end())
                base = now;
        }
    }
    if (base != nullptr)
        return base;
    // 因为外层循环的 base 到内层循环的边被删除了，因此假如找不到 base，可能因为这些边被删除了
    for (auto now : reserved) {
        for (auto next_raw : now->next) {
            auto next = next_raw->shared_from_this();
            if (set->find(next) != set->end()) {
                base = next;
            }
        }
    }
    return base;
}

// 对于所有函数：
// 1. 构建 cfg
// 2. 从大到小找出所有 scc
void loop_search::run() {
    auto func_list = m_->get_functions();
    for (auto func : func_list) {
        if (func->get_basic_blocks().size() == 0) {
            continue;
        }
        cfg_node_pointer_set nodes;
        cfg_node_pointer_set reserved;
        std::unordered_set<std::shared_ptr<cfg_node_pointer_set>> sccs;

        build_cfg(func, nodes);
        dump_graph(nodes, func->get_name());

        int scc_index = 0;
        while (scc(nodes, sccs)) {
            assert(sccs.size());
            for (auto scc : sccs) {
                auto base = find_loop_base(scc, reserved);

                // 根据 scc 构建对应 bb 的 scc
                auto bb_set = std::make_shared<bb_set_t>();
                for (auto now : *scc) {
                    bb_set->insert(now->bb);
                }
                loop_set.insert(bb_set);
                func2loop[func].insert(bb_set);
                // 只维护最低层次的对应
                base2loop.insert({base->bb, bb_set});
                loop2base.insert({bb_set, base->bb});

                // 将节点映射到最低层次的入口节点（因为是从大到小找 scc）
                for (auto bb : *bb_set) {
                    if (bb2base.find(bb) == bb2base.end())
                        bb2base.insert({bb, base->bb});
                    else
                        bb2base[bb] = base->bb;
                }

                // 将循环的入口节点删除，以便找出更小的 scc
                reserved.insert(base);
                dump_graph(*scc, func->get_name() + '_' + std::to_string(scc_index++));
                nodes.erase(base);
                for (auto next : base->next) {
                    next->pre.erase(base.get());
                }
                for (auto pre : base->pre) {
                    pre->next.erase(base.get());
                }
            }
            for (auto now : nodes) {
                now->dfn = now->low = -1;
                now->in_stack = false;
            }
            sccs.clear();
        }
        LOG_INFO << "Found " << scc_index << " scc in " << func->get_name();
    }
}

void loop_search::dump_graph(cfg_node_pointer_set &nodes, std::string title) {
    if (dump) {
        std::vector<std::string> edge_set;
        for (auto node : nodes) {
            if (node->bb->get_name() == "") {
                return;
            }
            if (base2loop.find(node->bb) != base2loop.end()) {
                for (auto succ_raw : node->next) {
                    auto succ = succ_raw->shared_from_this();
                    if (nodes.find(succ) != nodes.end()) {
                        edge_set.insert(edge_set.begin(),
                                        '\t' + node->bb->get_name() + "->" + succ->bb->get_name() + ';' + '\n');
                    }
                }
                edge_set.insert(edge_set.begin(), '\t' + node->bb->get_name() + " [color=red]" + ';' + '\n');
            } else {
                for (auto succ_raw : node->next) {
                    auto succ = succ_raw->shared_from_this();
                    if (nodes.find(succ) != nodes.end()) {
                        edge_set.push_back('\t' + node->bb->get_name() + "->" + succ->bb->get_name() + ';' + '\n');
                    }
                }
            }
        }
        std::string digragh = "digraph G {\n";
        for (auto edge : edge_set) {
            digragh += edge;
        }
        digragh += '}';
        std::ofstream debug_output;
        debug_output.open(title + ".dot", std::ios::out);

        debug_output << digragh;
        debug_output.close();
        std::string dot_cmd = "dot -Tpng " + title + ".dot" + " -o " + title + ".png";
        std::system(dot_cmd.c_str());
    }
}

// 找到当前内层循环的外层循环
std::shared_ptr<bb_set_t> loop_search::get_parent_loop(std::shared_ptr<bb_set_t> loop) {
    auto base = loop2base[loop];
    // 两个强联通分量之间要么不交，要么一个包含另外一个
    for (auto pre : base->get_pre_basic_blocks()) {
        // 需要寻找一个不在循环里的前驱，这个前驱所在的循环就是外层循环
        if (loop->find(pre) != loop->end())
            continue;
        auto outer = get_inner_loop(pre);
        if (outer == nullptr || outer->find(base) == outer->end())
            return nullptr;
        return outer;
    }
    return nullptr;
}

std::unordered_set<std::shared_ptr<bb_set_t>> loop_search::get_loops_in_func(std::shared_ptr<Function> f) {
    return func2loop.count(f) ? func2loop[f] : std::unordered_set<std::shared_ptr<bb_set_t>>();
}

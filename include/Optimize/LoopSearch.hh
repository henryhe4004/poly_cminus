#pragma once
#include "BasicBlock.h"
#include "Function.h"
#include "Pass.hh"
#include "logging.hpp"

#include <unordered_map>
#include <unordered_set>

struct cfg_node;
using cfg_node_pointer = std::shared_ptr<cfg_node>;
using bb_set_t = std::unordered_set<BasicBlock *>;
using cfg_node_pointer_set = std::unordered_set<cfg_node_pointer>;

// 统一使用智能指针
class loop_search : public Pass {
  public:
    explicit loop_search(Module *m, bool dump = false) : Pass(m), dump(dump) {}
    ~loop_search() = default;
    // 构建指定函数的 cfg
    void build_cfg(std::shared_ptr<Function> func, std::unordered_set<cfg_node_pointer> &result);
    void run() override;
    // 找出给定的 cfg 中的 scc
    bool scc(cfg_node_pointer_set &nodes, std::unordered_set<std::shared_ptr<cfg_node_pointer_set>> &result);
    // 调试函数
    void dump_graph(cfg_node_pointer_set &nodes, std::string title);
    // TODO 添加验证所求块是否为强联通分量的算法
    // 找出给定连通块中的 scc
    void traverse(cfg_node_pointer now, std::unordered_set<std::shared_ptr<cfg_node_pointer_set>> &result);
    // 找出 scc 中的入口点
    cfg_node_pointer find_loop_base(std::shared_ptr<cfg_node_pointer_set> set, cfg_node_pointer_set &reserved);

    // 利用iterator来遍历所有的loop
    auto begin() { return loop_set.begin(); }
    auto end() { return loop_set.end(); }

    // 找出给定的循环的入口
    BasicBlock *get_loop_base(std::shared_ptr<bb_set_t> loop) { return loop2base[loop]; }

    // 得到bb所在最低层次的loop
    std::shared_ptr<bb_set_t> get_inner_loop(BasicBlock *bb) {
        if (bb2base.find(bb) == bb2base.end())
            return nullptr;
        return base2loop[bb2base[bb]];
    }

    // 得到输入loop的外一层的循环，如果没有则返回空
    std::shared_ptr<bb_set_t> get_parent_loop(std::shared_ptr<bb_set_t> loop);

    // 得到函数 f 里的所有循环
    std::unordered_set<std::shared_ptr<bb_set_t>> get_loops_in_func(std::shared_ptr<Function> f);
    std::unordered_map<BasicBlock *, std::shared_ptr<bb_set_t>> &get_base2loop() { return base2loop; }

  private:
    bool dump;
    int index_count;
    std::vector<cfg_node_pointer> stack;
    // 找到的所有循环
    std::unordered_set<std::shared_ptr<bb_set_t>> loop_set;
    // 函数中找到的所有循环
    std::unordered_map<std::shared_ptr<Function>, std::unordered_set<std::shared_ptr<bb_set_t>>> func2loop;
    // { entry bb of loop : loop }
    std::unordered_map<BasicBlock *, std::shared_ptr<bb_set_t>> base2loop;
    // { loop : entry bb of loop }
    std::unordered_map<std::shared_ptr<bb_set_t>, BasicBlock *> loop2base;
    // { bb :  entry bb of loop} 默认最低层次的loop
    std::unordered_map<BasicBlock *, BasicBlock *> bb2base;
};

struct cfg_node : public std::enable_shared_from_this<cfg_node> {
    std::set<cfg_node *> next;
    std::set<cfg_node *> pre;
    BasicBlock *bb;
    int dfn;  // 节点在流图中的编号
    int low;  // scc 辅助变量
    bool in_stack;
    cfg_node(BasicBlock *bb, int dfn, int low, bool in_stack)
        : std::enable_shared_from_this<cfg_node>(), bb(bb), dfn(dfn), low(low), in_stack(in_stack) {}
};

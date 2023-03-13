#pragma once
#include "Dominators.hh"
#include "Function.h"
#include "LoopSearch.hh"
#include "Module.h"
#include "Value.h"
#include "val.hh"

#include <boost/icl/interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>
namespace icl = boost::icl;

class Codegen;

class RegAlloc {
  private:
    struct interval_ssa {
        using S = icl::interval_set<size_t>;
        using I = S::interval_type;
        S s;
        std::set<size_t> uses;
        std::list<std::pair<size_t, size_t>> ranges;
        Value *val;
        Reg reg{};
        std::string meta;
        void add_range(size_t start, size_t end) { s.insert(I::right_open(start, end)); }
        void add_use(size_t pos) { uses.insert(pos); }
        /// \brief interval set must be non-empty
        void set_from(size_t from) {
            if (not s.empty()) {
                auto lower = s.begin()->lower();
                s.erase(I::right_open(lower, from));
            }
        }
        const size_t start() const {
            auto lower = s.begin()->lower();
            return lower;
        }
        const size_t end() const {
            auto upper = s.rbegin()->upper();
            return upper;
        }
        const bool cover(size_t start, size_t end) const {
            auto b = icl::contains(s, I::closed(start, end));
            return b;
        }
        const bool cover(size_t pos) const {
            auto b = icl::contains(s, pos);
            return b;
        }
        /// \brief `this` has a lifetime hole during `other`
        /// \returns nullopt if no lifetime hole, position of intersection if intersect
        std::optional<size_t> intersect(const interval_ssa &other) {
            S intersection = s & other.s;
            LOG_DEBUG << "s: " << s << ", other.s: " << other.s << ", Intersection: " << intersection;
            if (intersection.empty())
                return {};
            else
                return intersection.begin()->lower();
        }
        /// \brief modifies `ranges` and `s`
        /// \returns second half of the split
        interval_ssa split(size_t pos) {
            LOG_DEBUG << "split: " << pos;
            //第一个大于等于pos的左边界
            auto i = uses.lower_bound(pos);
            std::set<size_t> retuses(i, uses.end());
            //删除i到uses》end的元素
            uses.erase(i, uses.end());
            S rets;
            S temp = I::right_open(0, pos) & s;
            //中间[pos,i-1]
            rets = s - temp;
            s = move(temp);

            LOG_DEBUG << "split " << val->get_name() << ": second half: " << rets << ", first half: " << s;
            interval_ssa ret(*this);
            ret.s = move(rets);
            ret.uses = move(retuses);
            ret.reg = Reg();
            return ret;
        }
        bool empty() const { return s.empty(); }
        bool operator<(const interval_ssa &other) const {
            return std::tie(s, val) <
                   std::tie(other.s, other.val);  // https://en.cppreference.com/w/cpp/utility/tuple/tie
        }
        friend std::ostream &operator<<(std::ostream &os, const interval_ssa &intv) {
            std::string log_reg;
            log_reg += intv.val->get_name() + ", " + intv.meta + ": ";
            std::stringstream ss;
            ss << intv.s << " uses: [";
            for (auto p : intv.uses)
                ss << p << ",";
            ss << "]";
            log_reg += ss.str();
            return os << log_reg;
        }
    };

    Module *m_;
    Function *f_;
    std::unique_ptr<loop_search> loops;
    int bb_index;
    std::map<BasicBlock *, int> bb2int;
    std::set<BasicBlock *> linear_set;  // for lookup, same element as linear_list
    std::list<std::shared_ptr<BasicBlock>> linear_list;
    std::map<BasicBlock *, int> visited;
    std::map<int, BasicBlock *> int2bb;

    std::map<BasicBlock *, std::set<Value *>> liveIn;
    std::size_t inst_count;
    std::map<BasicBlock *, std::pair<size_t, size_t>> bb_range;

    std::map<Reg, size_t> freeUntilPositionInteger;
    std::map<Reg, size_t> nextUsePosInteger;

    std::map<Reg, size_t> freeUntilPositionFloat;
    std::map<Reg, size_t> nextUsePosFloat;
    std::map<Function *, std::set<std::pair<size_t, Instruction *>>> call_pos;
    // register allocation result
  private:
    std::map<Value *, interval_ssa> ssa_intervals;
    std::map<Value *, std::vector<interval_ssa>> reg_mapping;
    std::map<Value *, std::vector<interval_ssa>> f_reg_mapping;
    std::map<std::pair<BasicBlock *, BasicBlock *>, std::vector<std::pair<armval, armval>>>
        move_mapping;  // move between phi and its operands
    std::map<std::pair<BasicBlock *, BasicBlock *>, std::vector<std::pair<Reg, size_t>>> bb_stores;
    std::map<std::pair<BasicBlock *, BasicBlock *>, std::vector<std::pair<size_t, Reg>>> bb_loads;
    std::vector<std::pair<Value *, size_t>> split_moves;
    std::map<Function *, std::vector<std::pair<Value *, size_t>>> f_split_moves;
    std::map<Function *, std::map<size_t, std::vector<std::pair<Reg, size_t>>>> fstores;
    std::map<Function *, std::map<size_t, std::vector<std::pair<size_t, Reg>>>> floads;
    std::map<Function *, std::map<size_t, std::vector<std::pair<Reg, Reg>>>> fmoves;
    std::map<Function *, std::map<size_t, std::vector<std::pair<armval, armval>>>> fordered_moves;

    std::map<size_t, std::vector<std::pair<Reg, size_t>>> stores;
    std::map<size_t, std::vector<std::pair<size_t, Reg>>> loads;
    std::map<size_t, std::vector<std::pair<Reg, Reg>>> moves;
    std::map<size_t, std::vector<std::pair<armval, armval>>> ordered_moves;
    std::map<Function *, size_t> stack_size;
    std::map<Value *, size_t> sp_offset;
    std::map<Instruction *, std::set<Reg>> call_saves;

  private:
    int reg_num;
    std::set<size_t> bb_starts;  // only used to check if a position is the start of bb, don't preserve order
    inline void spill(Value *v) {
        if (not sp_offset.count(v)) {  // allocate once
            sp_offset[v] = stack_size[f_];
            stack_size[f_] += (v->get_type()->get_size() < 8) ? 8 : v->get_type()->get_size();
        }
    }

  public:
    static const uint32_t int_arg_max_save_index = 16, fp_arg_max_save_index = 15;
    // Reg(16) = $r20 = $a8

    friend Codegen;
    RegAlloc(Module *, int reg_n = 10);
    void init_func();
    void LinearScanRegisterAllocation();
    void linear_scan_ssa();
    void resolve();
    void print_stats();
    void run();
    void dfs(BasicBlock *);
    void construct_linear_order();
    void construct_linear_order_recur(const bb_set_t &, BasicBlock *);
    /// \brief build live intervals and allocate stack space
    void build_intervals();
    void log_intervals();
    void check_intervals();
    Reg get(Value *, size_t index);
    armval get_loc(Value *, size_t);
};

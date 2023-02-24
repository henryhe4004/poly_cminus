#pragma once
#include "Module.h"
#include "instgen.hh"
#include "regalloc.hh"
#include "utils.hh"
#include "val.hh"

#include <fstream>
enum class move_t { ireg, freg, mem };

std::vector<std::pair<int, int>> topo_sort(std::set<int, std::greater<int>> &, std::multimap<int, int> &, move_t);

class Codegen {
  public:
    void run();
    Codegen(Module *m, std::ofstream &output, int reg_n) : m(m), output(output), reg_num(reg_n) {
        Instgen::output = &output;
    }

  private:
    int reg_num;

    size_t stack_growth_since_entry = 0;  // relative to the original sp value
    Module *m;
    std::map<Value *, std::vector<RegAlloc::interval_ssa>> reg_mapping;
    std::map<std::pair<BasicBlock *, BasicBlock *>, std::vector<std::pair<armval, armval>>> move_mapping;
    std::map<Function *, size_t> stack_size;
    std::map<Value *, size_t> sp_offset;
    std::map<size_t, std::vector<std::pair<Reg, size_t>>> stores;
    std::map<size_t, std::vector<std::pair<size_t, Reg>>> loads;
    std::map<size_t, std::vector<std::pair<Reg, Reg>>> moves;
    std::map<size_t, std::vector<std::pair<armval, armval>>> ordered_moves;
    std::map<Instruction *, std::set<Reg>> call_saves;
    std::ofstream &output;
    std::map<GlobalVariable *, int> GOT;
    const std::string global_vars_label = ".global_vars";

    size_t pos{0};                                       // tracking the index of the current instruction (order of dfs)
    std::map<Reg, RegAlloc::interval_ssa> caller_saved;  // currently in used caller-saved registers
    size_t callee_saved_size{};
    void gen_function(Function *f);
    void prologue(Function *f);
    void epilogue(Function *f);
    void gen_bb(BasicBlock *bb);
    void gen_binary(BinaryInst *);
    void gen_ternary(Instruction *);
    void gen_cmp(CmpInst *);
    void gen_fcmp(FCmpInst *);
    void gen_zext(ZextInst *);
    void gen_fptosi(FpToSiInst *);
    void gen_sitofp(SiToFpInst *);
    void gen_call(CallInst *);
    void gen_ret(ReturnInst *);
    void gen_br(BasicBlock *, BranchInst *);
    void gen_switch(BasicBlock*,SwitchInst *);
    void gen_select(SelectInst* sel);
    void gen_function_call(std::string func_name,
                           std::vector<Value *> all_value,
                           Instruction *dest_inst,
                           Instruction *push_pop_key = nullptr);
    void gen_got();
    void move(Reg, Value *, std::string_view cond = ""sv);
    void move(Value *, Value *, std::string_view cond = ""sv);
    void store(Value *, Value *, StoreInst *store);
    void load(Value *, Value *, LoadInst *load);
    void resolve_split(size_t, bool store = true, bool load = true, bool move = true);
    void resolve_moves(BasicBlock *frombb, BasicBlock *tobb);
    void resolve_moves(BasicBlock *frombb, BasicBlock *tobb, std::string_view cond);
    void parallel_mov(std::vector<std::pair<armval, armval>> &, std::string_view = ""sv);

    Reg get(Value *val, bool output = false) {
        auto p = pos + (output ? 1 : 0);
        auto intervals = reg_mapping.at(val);
        // FIXME: really slow lookup !!!~~~
        // but i guess we won't change it in the near future ()
        for (auto &interval : intervals) {
            if (interval.cover(p))
                return interval.reg;
        }
        if (not sp_offset.count(val))
            LOG_WARNING << "lifetime hole for value " << val->get_name() << " at pos " << pos << "\n";
        // exit(120);
        return Reg();
    }
    void get_reg_and_move(Reg &, Value *);
    armval get_loc(Value *val) {
        LOG_DEBUG << "getloc" << val->print();
        if (not(dynamic_cast<Instruction *>(val) or dynamic_cast<Argument *>(val)))
            return val;
        if (isa<AllocaInst>(val))
            return val;
        if (not reg_mapping.count(val)) {
            if (not val->get_type()->is_float_type())
                return armval{std::in_place_index<int_variant_index>, sp_offset.at(val)};
            return armval{std::in_place_index<float_variant_index>, sp_offset.at(val)};
        }
        Reg r = get(val);
        if (r.valid())
            return r;
        if (not val->get_type()->is_float_type())
            return armval{std::in_place_index<int_variant_index>, sp_offset.at(val)};
        return armval{std::in_place_index<float_variant_index>, sp_offset.at(val)};
    }

  public:
    static const Reg temp_lhs_reg, temp_rhs_reg, temp_out_reg;
    static const Reg temp_float_lhs_reg, temp_float_rhs_reg;
    static const uint32_t imm16 = 1 << 16;
    static const uint32_t imm8 = 1 << 8;

    static void move(Reg, int, std::string_view cond = ""sv);
};

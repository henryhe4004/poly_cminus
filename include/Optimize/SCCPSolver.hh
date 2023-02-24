#pragma once

#include "ConstFold.hh"
#include "Pass.hh"
#include "errorcode.hh"
#include "utils.hh"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

/**
 * 格值
 * TODO: 没有处理 undef 的情况
 */
class Lattice {
  public:
    enum class LatticeType { unknown, constant, overdefined } tag;
    std::shared_ptr<Constant> val;
    Lattice() : tag(LatticeType::unknown), val(nullptr) {}

    bool is_unknown() const { return tag == LatticeType::unknown; }
    bool is_constant() const { return tag == LatticeType::constant; }
    bool is_overdefined() const { return tag == LatticeType::overdefined; }
    bool mark_unknown() {
        if (is_unknown())
            return false;
        tag = LatticeType::unknown;
        return true;
    }
    bool mark_overdefined() {
        if (is_overdefined())
            return false;
        tag = LatticeType::overdefined;
        return true;
    }
    bool mark_constant(std::shared_ptr<Constant> newval) {
        if (is_constant()) {
            exit_if(newval != val, ERROR_IN_SCCP, "SCCP: mark a constant with different constant");
            return false;
        }
        tag = LatticeType::constant;
        val = newval;
        return true;
    }

    bool mergein(const Lattice &rhs) {
        if (rhs.is_unknown() or is_overdefined())
            return false;
        if (rhs.is_overdefined()) {
            mark_overdefined();
            return true;
        }
        if (is_unknown()) {
            exit_if(rhs.is_unknown(), ERROR_IN_SCCP, "SCCP: try to mergein unknown");
            *this = rhs;
            return true;
        }
        if (is_constant()) {
            if (rhs.is_constant() and val == rhs.val)
                return false;
            mark_overdefined();
            return true;
        }
        exit(ERROR_IN_SCCP, "SCCP: unhandle state in mergein");
        return false;
    }

    std::string print() {
        if (is_unknown())
            return "unknown";
        if (is_overdefined())
            return "overdefined";
        if (isa<ConstantInt>(val))
            return "const int (" + std::to_string(std::dynamic_pointer_cast<ConstantInt>(val)->get_value()) + ")";
        return "const int (" + std::to_string(std::dynamic_pointer_cast<ConstantFP>(val)->get_value()) + ")";
    }
};

/**
 * SCCP 的辅助类，用来计算值的格值
 */
class SCCPSolver {
  public:
    SCCPSolver(Module *m, std::shared_ptr<Function> func)
        : func(func), const_fold(std::make_shared<ConstFold>(m)), m_(m) {}

    bool mark_block_executable(std::shared_ptr<BasicBlock> bb) {
        if (not bb_executable.insert(bb).second)
            return false;
        bb_worklist.push_back(bb);
        return true;
    }
    bool mark_overdefined(std::shared_ptr<Value> val) { return mark_overdefined(value_state[val], val); }
    bool mark_overdefined(Lattice &lat, std::shared_ptr<Value> val) {
        if (not lat.mark_overdefined())
            return false;
        push_to_worklist(lat, val);
        return true;
    }
    void push_to_worklist(Lattice &lat, std::shared_ptr<Value> val) {
        if (lat.is_overdefined())
            return overdefined_inst_worklist.push_back(val);
        inst_worklist.push_back(val);
    }
    void solve() {
        while (bb_worklist.empty() == false or inst_worklist.empty() == false or
               overdefined_inst_worklist.empty() == false) {
            while (not overdefined_inst_worklist.empty()) {
                auto val = overdefined_inst_worklist.front();
                overdefined_inst_worklist.pop_front();
                mark_users_as_changed(val);
            }
            while (not inst_worklist.empty()) {
                auto val = inst_worklist.front();
                inst_worklist.pop_front();
                if (get_value_state(val).is_overdefined())
                    mark_users_as_changed(val);
            }
            while (not bb_worklist.empty()) {
                auto bb = bb_worklist.front();
                bb_worklist.pop_front();
                visit(bb);
            }
        }
    }
    void log() {
        for (auto bb : func->get_basic_blocks()) {
            if (not is_block_executable(bb))
                LOG_INFO << "bb not executable: " << bb->get_name();
            else
                LOG_INFO << "bb executable" << bb->get_name();
            for (auto inst : bb->get_instructions()) {
                if (inst->is_void())
                    continue;
                if (value_state.find(inst) != value_state.end())
                    LOG_INFO << "SCCP: val: " << inst->print() << " lat: " << value_state[inst].print();
                else
                    LOG_INFO << "SCCP: val: " << inst->print() << " lat: undef";
            }
        }
    }

    void mark_users_as_changed(std::shared_ptr<Value> val) {
        exit_if(isa<Function>(val), ERROR_IN_SCCP, "SCCP: mark a function as changed");
        for (auto user : val->get_use_list()) {
            if (auto inst_ptr = std::dynamic_pointer_cast<Instruction>(user.val_->shared_from_this()))
                operand_changed_state(inst_ptr);
        }
    }
    void operand_changed_state(std::shared_ptr<Instruction> inst) {
        if (bb_executable.find(inst->get_parent_shared()) != bb_executable.end())
            visit(inst);
    }
    Lattice &get_value_state(std::shared_ptr<Value> val) {
        if (value_state.find(val) != value_state.end())
            return value_state[val];
        Lattice &lat = value_state[val];
        if (auto constptr = std::dynamic_pointer_cast<Constant>(val))
            lat.mark_constant(constptr);
        return lat;
    }
    // void check_undef();
    bool is_block_executable(std::shared_ptr<BasicBlock> bb) { return bb_executable.find(bb) != bb_executable.end(); }
    void visit(std::shared_ptr<BasicBlock> bb) {
        for (auto inst : bb->get_instructions())
            visit(inst);
    }
    void visit(std::shared_ptr<Instruction> inst) {
        switch (inst->get_instr_type()) {
            case Instruction::phi:
                visit_phi(std::dynamic_pointer_cast<PhiInst>(inst));
                break;
            case Instruction::add:
            case Instruction::sub:
            case Instruction::mul:
            case Instruction::sdiv:
            case Instruction::srem:
            case Instruction::shl:
            case Instruction::lshr:
            case Instruction::ashr:
            case Instruction::and_:
                visit_binary(std::dynamic_pointer_cast<BinaryInst>(inst));
                break;
            case Instruction::fadd:
            case Instruction::fsub:
            case Instruction::fmul:
            case Instruction::fdiv:
                visit_binary(std::dynamic_pointer_cast<BinaryInst>(inst));
                break;
            case Instruction::alloca:
                visit_alloca(std::dynamic_pointer_cast<AllocaInst>(inst));
                break;
            case Instruction::load:
                visit_load(std::dynamic_pointer_cast<LoadInst>(inst));
                break;
            case Instruction::store:
                visit_store(std::dynamic_pointer_cast<StoreInst>(inst));
                break;
            case Instruction::cmp:
                visit_cmp(std::dynamic_pointer_cast<CmpInst>(inst));
                break;
            case Instruction::fcmp:
                visit_fcmp(std::dynamic_pointer_cast<FCmpInst>(inst));
                break;
            case Instruction::call:
                visit_call(std::dynamic_pointer_cast<CallInst>(inst));
                break;
            case Instruction::getelementptr:
                visit_gep(std::dynamic_pointer_cast<GetElementPtrInst>(inst));
                break;
            case Instruction::zext:
                visit_zext(std::dynamic_pointer_cast<ZextInst>(inst));
                break;
            case Instruction::fptosi:
                visit_fptosi(std::dynamic_pointer_cast<FpToSiInst>(inst));
                break;
            case Instruction::sitofp:
                visit_sitofp(std::dynamic_pointer_cast<SiToFpInst>(inst));
                break;
            case Instruction::ptrtoint:
                visit_ptrtoint(std::dynamic_pointer_cast<PtrToIntInst>(inst));
                break;
            case Instruction::inttoptr:
                visit_inttoptr(std::dynamic_pointer_cast<IntToPtrInst>(inst));
                break;
            case Instruction::ret:
                visit_ret(std::dynamic_pointer_cast<ReturnInst>(inst));
                break;
            case Instruction::br:
                visit_br(std::dynamic_pointer_cast<BranchInst>(inst));
                break;
            default:
                exit(ERROR_IN_SCCP, "Unexceped inst type in sccp");
        }
    }
    void visit_phi(std::shared_ptr<PhiInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return;

        Lattice lat = get_value_state(inst);
        for (int i = 1; i < inst->get_num_operand(); i += 2) {
            if (not is_edge_feasible(std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(i)->shared_from_this()),
                                     inst->get_parent_shared()))
                continue;

            Lattice rhs = get_value_state(inst->get_operand(i - 1)->shared_from_this());
            lat.mergein(rhs);
            if (lat.is_overdefined())
                break;
        }
        mergein_value(inst, lat);
    }
    bool mergein_value(Lattice &lhs, Lattice rhs, std::shared_ptr<Value> val) {
        if (lhs.mergein(rhs)) {
            push_to_worklist(lhs, val);
            return true;
        }
        return false;
    }
    bool mergein_value(std::shared_ptr<Instruction> inst, Lattice lat) {
        return mergein_value(get_value_state(inst), lat, inst);
    }
    bool is_edge_feasible(std::shared_ptr<BasicBlock> from, std::shared_ptr<BasicBlock> to) {
        return executable_edges.find(edge(from, to)) != executable_edges.end();
    }
    bool mark_edge_executable(std::shared_ptr<BasicBlock> from, std::shared_ptr<BasicBlock> to) {
        if (not executable_edges.insert(edge(from, to)).second)
            return false;
        if (not mark_block_executable(to)) {
            for (auto it = to->get_instructions().begin(); it != to->get_instructions().end() and (*it)->is_phi(); ++it)
                visit_phi(std::dynamic_pointer_cast<PhiInst>(*it));
        }
        return true;
    }
    bool process_unknown() {
        bool changed = false;
        for (auto bb : func->get_basic_blocks()) {
            if (not is_block_executable(bb))
                continue;
            for (auto inst : bb->get_instructions()) {
                if (inst->is_void())
                    continue;
                if (get_value_state(inst).is_unknown()) {
                    mark_overdefined(inst);
                    changed = true;
                }
            }
            if (auto br_inst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator())) {
                if (br_inst->is_cond_br() == false)
                    continue;
                auto cond = br_inst->get_operand(0)->shared_from_this();
                // 随便定一个向，这种情况只会出现在循环
                if (get_value_state(cond).is_unknown()) {
                    mark_edge_executable(
                        bb, std::dynamic_pointer_cast<BasicBlock>(br_inst->get_operand(1)->shared_from_this()));
                    changed = true;
                }
            }
        }
        return changed;
    }
    // 我们的类层次还缺少转换类型的指令
    void visit_binary(std::shared_ptr<BinaryInst> inst) {
        Lattice llat = get_value_state(inst->get_operand(0)->shared_from_this());
        Lattice rlat = get_value_state(inst->get_operand(1)->shared_from_this());

        Lattice &now = get_value_state(inst);
        if (now.is_overdefined())
            return;
        if (llat.is_unknown() or rlat.is_unknown())
            return;

        if (llat.is_overdefined() and rlat.is_overdefined())
            return (void)mark_overdefined(inst);

        if (llat.is_constant() or rlat.is_constant()) {
            std::shared_ptr<Value> lval = llat.is_constant() ? llat.val : inst->get_operand(0)->shared_from_this();
            std::shared_ptr<Value> rval = rlat.is_constant() ? rlat.val : inst->get_operand(1)->shared_from_this();
            auto result = const_fold->eval(inst->get_instr_type(), lval, rval);
            if (auto const_result = std::dynamic_pointer_cast<Constant>(result)) {
                Lattice new_lat;
                new_lat.mark_constant(const_result);
                return (void)mergein_value(inst, new_lat);
            }
        }

        // 剩下的情况只有一个是常数一个是 overdefined 了
        mark_overdefined(inst);
    }
    void visit_cmp(std::shared_ptr<CmpInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return (void)mark_overdefined(inst);

        auto lval = inst->get_operand(0)->shared_from_this();
        auto rval = inst->get_operand(1)->shared_from_this();

        auto llat = get_value_state(lval);
        auto rlat = get_value_state(rval);

        if (llat.is_unknown() or rlat.is_unknown())
            return;
        if (llat.is_constant() and rlat.is_constant()) {
            auto result = const_fold->eval(inst->get_cmp_op(), llat.val, rlat.val);
            exit_if(isa<Constant>(result) == false, ERROR_IN_SCCP, "SCCP: result should be constant");
            Lattice new_lat;
            new_lat.mark_constant(std::dynamic_pointer_cast<Constant>(result));
            mergein_value(inst, new_lat);
            return;
        }
        mark_overdefined(inst);
    }
    void visit_fcmp(std::shared_ptr<FCmpInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return (void)mark_overdefined(inst);

        auto lval = inst->get_operand(0)->shared_from_this();
        auto rval = inst->get_operand(1)->shared_from_this();

        auto llat = get_value_state(lval);
        auto rlat = get_value_state(rval);

        if (llat.is_unknown() or rlat.is_unknown())
            return;
        if (llat.is_constant() and rlat.is_constant()) {
            auto result = const_fold->eval(inst->get_cmp_op(), llat.val, rlat.val);
            exit_if(isa<Constant>(result) == false, ERROR_IN_SCCP, "SCCP: result should be constant");
            Lattice new_lat;
            new_lat.mark_constant(std::dynamic_pointer_cast<Constant>(result));
            mergein_value(inst, new_lat);
            return;
        }
        mark_overdefined(inst);
    }
    void visit_call(std::shared_ptr<CallInst> inst) {
        // 对于纯函数可以直接求值，但是需要一个解释器
        // 因为我们没有对于数组元素的常数传播，因此不需要考虑函数传入数组地址的问题
        mark_overdefined(inst);
    }
    void visit_br(std::shared_ptr<BranchInst> inst) {
        if (inst->is_cond_br()) {
            auto cond = inst->get_operand(0)->shared_from_this();
            auto lat = get_value_state(cond);
            if (lat.is_constant()) {
                exit_if(isa<ConstantInt>(lat.val) == false,
                        ERROR_IN_SCCP,
                        "SCCP: condition is not constant int, it is " + lat.val->print());
                auto val = std::dynamic_pointer_cast<ConstantInt>(lat.val)->get_value();
                if (val)
                    mark_edge_executable(
                        inst->get_parent_shared(),
                        std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(1)->shared_from_this()));
                else
                    mark_edge_executable(
                        inst->get_parent_shared(),
                        std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(2)->shared_from_this()));
            } else if (lat.is_overdefined()) {
                mark_edge_executable(inst->get_parent_shared(),
                                     std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(1)->shared_from_this()));
                mark_edge_executable(inst->get_parent_shared(),
                                     std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(2)->shared_from_this()));
            }  // unknown 时：do nothing
        } else {
            auto succ = std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(0)->shared_from_this());
            mark_edge_executable(inst->get_parent_shared(), succ);
        }
    }
    void visit_ret(std::shared_ptr<ReturnInst> inst) { return; }
    // 常数 gep 是可以计算出来的，但是缺少重写的方法
    void visit_gep(std::shared_ptr<GetElementPtrInst> inst) { mark_overdefined(inst); }
    // 局部数组变量交给 BB constant 去追踪，全局数组变量暂时没有常数传播的想法
    void visit_store(std::shared_ptr<StoreInst> inst) { return; }
    // 同前，对于 load 不进行追踪
    void visit_load(std::shared_ptr<LoadInst> inst) { mark_overdefined(inst); }
    // alloc 得到的地址也不被当做是常数
    void visit_alloca(std::shared_ptr<AllocaInst> inst) { mark_overdefined(inst); }
    void visit_ptrtoint(std::shared_ptr<PtrToIntInst> inst) { mark_overdefined(inst); }
    // 这里可以优化，但是缺少重写的方法
    void visit_inttoptr(std::shared_ptr<IntToPtrInst> inst) { mark_overdefined(inst); }
    void visit_zext(std::shared_ptr<ZextInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return;
        auto op_lat = get_value_state(inst->get_operand(0)->shared_from_this());
        if (op_lat.is_unknown())
            return;
        if (op_lat.is_overdefined())
            return (void)mark_overdefined(inst);
        exit_if(isa<ConstantInt>(op_lat.val) == false, ERROR_IN_SCCP, "SCCP: error in zext");
        auto val = std::dynamic_pointer_cast<ConstantInt>(op_lat.val)->get_value();
        auto to_type = inst->get_dest_type();
        exit_if(to_type->is_integer_type() == false, ERROR_IN_SCCP, "SCCP: zext to non-integer type");

        std::shared_ptr<ConstantInt> result;
        switch (to_type->get_size()) {
            case 1:
                result = ConstantInt::get(static_cast<bool>(val), m_);
                break;
            case 4:
                result = ConstantInt::get(static_cast<int>(val), m_);
                break;
            case 8:
                result = ConstantInt::get_i64(val, m_);
                break;
            default:
                exit(ERROR_IN_SCCP, "SCCP: error in zext constant");
        }
        mark_constant(inst, result);
    }
    void visit_fptosi(std::shared_ptr<FpToSiInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return;
        auto op_lat = get_value_state(inst->get_operand(0)->shared_from_this());
        if (op_lat.is_unknown())
            return;
        if (op_lat.is_overdefined())
            return (void)mark_overdefined(inst);
        exit_if(isa<ConstantFP>(op_lat.val) == false, ERROR_IN_SCCP, "SCCP: error in fptosi");
        auto val = std::dynamic_pointer_cast<ConstantFP>(op_lat.val)->get_value();
        mark_constant(inst, ConstantInt::get(static_cast<int>(val), m_));
    }
    void visit_sitofp(std::shared_ptr<SiToFpInst> inst) {
        if (get_value_state(inst).is_overdefined())
            return;
        auto op_lat = get_value_state(inst->get_operand(0)->shared_from_this());
        if (op_lat.is_unknown())
            return;
        if (op_lat.is_overdefined())
            return (void)mark_overdefined(inst);
        exit_if(isa<ConstantInt>(op_lat.val) == false, ERROR_IN_SCCP, "SCCP: error in sitofp");
        auto val = std::dynamic_pointer_cast<ConstantInt>(op_lat.val)->get_value();
        mark_constant(inst, ConstantFP::get(static_cast<float>(val), m_));
    }
    bool mark_constant(Lattice &lat, std::shared_ptr<Value> val, std::shared_ptr<Constant> C) {
        if (not lat.mark_constant(C))
            return false;
        push_to_worklist(lat, val);
        return true;
    }
    bool mark_constant(std::shared_ptr<Value> val, std::shared_ptr<Constant> C) {
        return mark_constant(get_value_state(val), val, C);
    }

  private:
    std::shared_ptr<Function> func;
    std::unordered_set<std::shared_ptr<BasicBlock>> bb_executable;
    std::deque<std::shared_ptr<BasicBlock>> bb_worklist;
    std::deque<std::shared_ptr<Value>> inst_worklist;
    std::deque<std::shared_ptr<Value>> overdefined_inst_worklist;
    std::unordered_map<std::shared_ptr<Value>, Lattice> value_state;
    std::shared_ptr<ConstFold> const_fold;
    using edge = std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<BasicBlock>>;
    struct edge_hash {
        std::size_t operator()(const edge &now) const {
            auto lhs = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(now.first.get()));
            auto rhs = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(now.second.get()));
            return lhs ^ rhs;
        }
    };
    std::unordered_set<edge, edge_hash> executable_edges;
    Module *m_;
};

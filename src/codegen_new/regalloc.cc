#include "regalloc.hh"
#include "Function.h"
#include "LoopSearch.hh"
#include "lir.h"
#include "utils.hh"
#include "val.hh"

#include <algorithm>
#include <functional>
#include <limits>

void RegAlloc::build_intervals() {
    bb_range.clear();
    bb_starts.clear();
    auto &fbbs = f_->get_basic_blocks();
    for (auto it = fbbs.rbegin(); it != fbbs.rend(); ++it) {
        auto bb = it->get();
        //开始计算inst_count的时候*2 所以减去的时候也*2
        const auto from = inst_count - 2 * bb->get_num_inst();
        auto to = inst_count;
        LOG_DEBUG<<"bb: "<<bb->get_name()<<" from: "<<from<<" to: "<<to;
        bb_range[bb] = {from, to};
        //排序需要的起始点
        bb_starts.insert(from);
        std::set<Value *> live{};
        // 入口处出发能够出现一个use（在def之前） 所有后继的liveIn就是该处的liveOut
        for (auto succ_bb : bb->get_succ_basic_blocks()){
            live.insert(liveIn[succ_bb].begin(), liveIn[succ_bb].end());
        }
        LOG_DEBUG<<"live in: ";
        for(auto i:live){
            LOG_DEBUG<<i->print();
        }

        // we really could use c++20 ranges
        for (auto succ_bb : bb->get_succ_basic_blocks()) {
            for (auto instr : succ_bb->get_instructions()) {
                if (not instr->is_phi())
                    break;
                auto phi = std::dynamic_pointer_cast<PhiInst>(instr);
                for (int i = 0; i < phi->get_num_operand(); i += 2) {
                    if (phi->get_operand(i + 1).get() == bb) {
                        // succ_bb: %output = phi[op, bb]
                        auto op = phi->get_operand(i).get();  
                        if (op->get_name().empty())
                            continue;
                        LOG_DEBUG<<"live phi op name:"<<op->get_name()<<" range from:"<<from<<" range to:"<<to<<" use: "<<to-1;
                        // used at `to`, but we add an right_open interval so plus 1
                        ssa_intervals[op].add_range(from, to);
                        ssa_intervals[op].add_use(to - 1);  // does input of phi count as a use?
                        live.insert(op);
                    }
                }
            }
        }
        auto insts = bb->get_instructions();
        for (auto val : live) {
            LOG_DEBUG<<"val: "<<val->print()<<" from:"<<from<<" to:"<<to;
            ssa_intervals[val].add_range(from, to);
        }
        //倒序计算指令的活跃区间
        for (auto it = insts.rbegin(); it != insts.rend(); ++it, inst_count -= 2) {
            auto inst = it->get();
            //((op_id_ == ret) || (op_id_ == br) || (op_id_ == store) || (op_id_ == mov) ||(op_id_ == call && this->get_type()->is_void_type())) 只有call指令有void_type
            if (not inst->get_type()->is_void_type() and live.count(inst)) {
                // phi starts from `from`+1 TODO: changed
                if(inst->is_phi()){
                    LOG_DEBUG<<"inst: "<<inst->print()<<"use and from:"<<from;
                }else{
                    LOG_DEBUG<<"inst: "<<inst->print()<<"use and from:"<<inst_count-1;
                }
                ssa_intervals[inst].set_from(inst->is_phi() ? from : inst_count - 1);
                ssa_intervals[inst].add_use(inst->is_phi() ? from : inst_count - 1);
                ssa_intervals[inst].val = inst;
                ssa_intervals[inst].meta += inst->get_type()->print() + ", " + bb->get_parent()->get_name();
                live.erase(inst);
            }
            if (inst->is_alloca()) {
                LOG_DEBUG << "allocate size: " << inst->get_type()->get_size();
                //sp_offset[inst]记录分配前起始
                sp_offset[inst] = stack_size[f_];
                //函数f_的栈大小扩大
                stack_size[f_] += static_cast<PointerType *>(inst->get_type())->get_size();
                ssa_intervals.erase(inst);
                continue;
            }
            if (inst->is_phi())
                continue;
            if (inst->is_call() or inst->is_div() or inst->is_srem())
                call_pos[f_].insert({inst_count - 1, inst});
            // TODO: do it in IR lowering
            if (inst->is_load() or inst->is_store()) {
                Value *ptr = inst->is_load() ? inst->get_operand(0).get() : inst->get_operand(1).get();
                Value *v1, *v2;
                AllocaInst *alloca;
                ConstantInt *ci;
                if (match(ptr, m_inttoptr(m_Add(m_ptrtoint(m_alloca(alloca)), m_constantInt(ci))))) {
                    LOG_DEBUG << "sp offset (not yet allocated ...) "
                              << ", add " << ci->get_value();
                    LOG_DEBUG << "match(ptr, m_inttoptr(m_Add(m_ptrtoint(m_alloca(alloca)), m_constantInt(ci))))";
                } else if (match(ptr, m_inttoptr(m_ptrtoint(m_value(v1))))) {
                    LOG_DEBUG << "match(ptr, m_inttoptr(m_ptrtoint(m_value(v1))))";
                }
            }
            for (auto jjjj : inst->get_operands()) {
                auto op = jjjj.get();
                if (op->get_name().empty())  // constants
                    continue;
                if (op->get_type()->is_integer_type() or op->get_type()->is_float_type() or
                    (op->get_type()->is_pointer_type() and !dynamic_cast<AllocaInst *>(op) and
                     !dynamic_cast<GlobalVariable *>(op))) {
                    LOG_DEBUG<<"op:"<<op->print()<<" from:"<<from<<"  to"<<inst_count-1<<" use"<<inst_count-2;
                    ssa_intervals[op].add_range(from, inst_count - 1);
                    ssa_intervals[op].add_use(inst_count - 2);
                    live.insert(op);
                }
            }
            if (inst->is_cmp() or inst->is_fcmp()) {
                bool must_save = false;  // 如果不是马上就使用该比较结果，需要保存在给inst分配好的寄存器中
                // 或者在寄存器分配的时候，如果无需保存，就不分配了
                auto next = (++inst->get_iterator())->get();
                for (auto use : inst->get_use_list()) {
                    if ((match(use.val_, m_br(m_specific(inst))) and next == use.val_) or
                        (match(use.val_, m_zext(m_specific(inst))) and next == use.val_))
                        continue;
                    must_save = true;
                    break;
                }
                if (not must_save)
                    ssa_intervals.erase(inst);
            }
        }
        //删除phi的活跃变量
        for (auto instr : insts) {
            if (not instr->is_phi())
                continue;
            LOG_DEBUG<<"delete live phi inst: "<<instr->print();
            live.erase(instr.get());
        }
        auto base2loop = loops->get_base2loop();
        if (base2loop.count(bb)) {
            auto &loopset = *base2loop.at(bb);
            auto endbb = *std::max_element(loopset.begin(), loopset.end(), [this](auto bb1, auto bb2) {
                return bb_range[bb1].second < bb_range[bb2].second;
            });
            auto [_, loop_end_index] = bb_range[endbb];
            LOG_DEBUG <<"bb"<<bb->get_name()<< " is loop header, extending live interval to " << loop_end_index;
            // FIXME: erroneous liveness analysis, see sec 4.3 Irreducible Control Flow
            // in short, add pseudo-phi instruction at loop header for the lives at header.liveIn
            for (auto op : live) {
                // if (!(std::all_of(loopset.begin(), loopset.end(), [&, this](auto block) {
                //     return block == bb or liveIn[block].count(op)) exit(13);
                // }));
                LOG_DEBUG<<"loop op: "<<op->print()<<"  from: "<<from<<" to: "<<loop_end_index;
                ssa_intervals[op].add_range(from, loop_end_index);
            }
            for (auto block : loopset) {
                if (block != bb)
                    liveIn[block].insert(live.begin(), live.end());
            }
        }
        liveIn[bb] = move(live);
    }
    int arg_idx = 0;  // TODO allocate fixed interval
    int arg_idx_float = 0;
    for (auto arg : f_->get_args()) {
        auto a = arg.get();
        auto is_float = a->get_type()->is_float_type();

        if (not a->get_use_list().empty()) {
            ssa_intervals[a].val = a;
            ssa_intervals[a].add_use(0);
            if (not is_float) {
                if (arg_idx <= int_arg_max_save_index)
                    LOG_DEBUG<<"alloca reg"<<"   "<<arg_idx;
                    ssa_intervals[a].reg = Reg(arg_idx);
            } else {
                if (arg_idx_float <= fp_arg_max_save_index)
                    ssa_intervals[a].reg = Reg(arg_idx_float, true);
            }
            ssa_intervals[a].meta = a->get_type()->print() + ", " + f_->get_name();
        }
        if (not is_float)
            arg_idx++;
        else
            arg_idx_float++;
    }
}

RegAlloc::RegAlloc(Module *m, int reg_n) : m_(m), reg_num(reg_n) {}

void RegAlloc::linear_scan_ssa() {
    f_reg_mapping.clear();
    std::set<interval_ssa> unhandled, active, inactive, handled;
    //占有寄存器的区间集合 若还未占有 则用unhandled存储起来
    for (auto [_, interval] : ssa_intervals) {
       
        if (interval.reg.valid()) {
            active.insert(interval);
            continue;
        }
        unhandled.insert(interval);
    }
    //处理还未占有寄存器的
    while (not unhandled.empty()) {
        //还未占有寄存器列表的第一个寄存器的活跃区间左边界
        auto current = *unhandled.begin();
        unhandled.erase(unhandled.begin());
        LOG_DEBUG << current;
        if (current.reg.valid()) {  // function arguments, already allocated
            // reg_mapping[current.val].push_back(current);
            active.insert(current);
            continue;
        }
        const auto position = current.start();
        // LOG_DEBUG << "checking for intervals in active that are handled or inactive";
        
        for (auto itit = active.begin(); itit != active.end();) {
            
            auto it = *itit;
            LOG_DEBUG<<it.val->print()<<" "<<it.start()<<" "<<it.end();
            //如果某活跃区间已经结束 则释放其占有的寄存器
            if (it.end() <= position) {
                itit = active.erase(itit);
                handled.insert(it);
                // reg_mapping[it.val].push_back(it);
                continue;
            }
            //如果position并没包括在某活跃区间 也就是position<it.begin();
            if (not it.cover(position)) {
                itit = active.erase(itit);
                inactive.insert(it);
                LOG_DEBUG<<position<<" "<<it;
                continue;
            }
            ++itit;
        }
        // LOG_DEBUG << "checking for intervals in inactive that are handled or active";
        for (auto itit = inactive.begin(); itit != inactive.end();) {
            auto it = *itit;
            if (it.end() < position) {
                itit = inactive.erase(itit);
                handled.insert(it);
                // reg_mapping[it.val].push_back(it);
                continue;
            }
            if (it.cover(position)) {
                LOG_DEBUG<<position<<" "<<it.start()<<" "<<it.end();
                itit = inactive.erase(itit);

                active.insert(it);
                continue;
            }
            LOG_DEBUG<<position<<" "<<it.start()<<" "<<it.end();
            ++itit;
        }
        bool failed = false;
        // LOG_DEBUG << "try allocate free register";

        auto is_float = current.val->get_type()->is_float_type();
        auto &freeUntilPosition = is_float ? freeUntilPositionFloat : freeUntilPositionInteger;
        auto &nextUsePos = is_float ? nextUsePosFloat : nextUsePosInteger;
        //确保判断float/int对应正确 若不正确 抛出异常
        if (!(std::all_of(freeUntilPosition.begin(), freeUntilPosition.end(), [&](auto &tuple) {
                auto &[r, p] = tuple;
                return r.is_float == is_float;
            })))
            exit(14);
        if (!(std::all_of(nextUsePos.begin(), nextUsePos.end(), [&](auto &tuple) {
                auto &[r, p] = tuple;
                return r.is_float == is_float;
            })))
            exit(15);
        for (auto &[r, p] : freeUntilPosition){
        //数值类型的最大值
            p = std::numeric_limits<size_t>::max();
        }
        for (auto it : inactive) {
            //交集
            auto intersect = it.intersect(current);
             // 交集是否有
            if (intersect.has_value()) {
                LOG_DEBUG<<it.reg<<" "<<it.val->print();
                if (not it.reg.is_float){
                    LOG_DEBUG<<freeUntilPositionInteger[it.reg]<<" "<<intersect.value();
                    freeUntilPositionInteger[it.reg] = std::min(freeUntilPositionInteger[it.reg], intersect.value());
                }
                else{
                    LOG_DEBUG<<freeUntilPositionFloat[it.reg]<<" "<<intersect.value();
                    freeUntilPositionFloat[it.reg] = std::min(freeUntilPositionFloat[it.reg], intersect.value());
                }
            }
        }
        //活跃变量的position为0 后面无法找到
        for (auto it : active) {
            LOG_DEBUG<<it.reg<<" "<<it.val->print();
            if (not it.reg.is_float)
                freeUntilPositionInteger[it.reg] = 0;
            else
                freeUntilPositionFloat[it.reg] = 0;
        }
        size_t pos;
        Reg reg;

        // find highest free positionprocess onlun
        //没有编译器
        if (freeUntilPosition.empty()){
            LOG_DEBUG<<"freeUntilPostion empty"<<pos;
            pos = 0;
        }else{
            for(auto [key,value] : freeUntilPosition){
                LOG_DEBUG<<key<<" "<<value;
            }
            std::tie(reg, pos) = *std::max_element(freeUntilPosition.begin(),
                                                   freeUntilPosition.end(),
                                                   [](auto &lhs, auto &rhs) { return lhs.second < rhs.second; });
            LOG_DEBUG<<pos;
        }
        //没有可用寄存器
        if (pos == 0) {
            failed = true;
            
        } else if (current.end() < pos) {//当前寄存器完全够用
            LOG_DEBUG << reg << " available for the whole interval";
            current.reg = reg;
            // reg_mapping[current.val].push_back(current);
        } else {//寄存器不够用 将原来的interval拆分（split)
            LOG_DEBUG << reg << " available for the first part of the interval";
            current.reg = reg;
            auto split = current.split(pos);
            unhandled.insert(split);
            if (bb_starts.count(pos))
                LOG_DEBUG << "split at beginning of bb (" << pos << "), not pushing to `split_moves`";
            else
                split_moves.push_back({current.val, pos});
        }
        //pos=0;
        if (failed) {
            for (auto &[r, p] : nextUsePos)
                p = std::numeric_limits<size_t>::max();
            // next use of it after start of current (considering infinite loop, better use lower_bound) 找到该变量每次使用的位置
            for (auto it : active)
                if (it.uses.lower_bound(position) != it.uses.end() and it.reg.is_float == is_float){
                    LOG_DEBUG<<nextUsePos[it.reg]<<" "<<*it.uses.lower_bound(position);
                    nextUsePos[it.reg] = std::min(nextUsePos[it.reg], *it.uses.lower_bound(position));
                    LOG_DEBUG<<it.reg<<" "<<nextUsePos[it.reg];
                }
            for (auto it : inactive)
                if (it.uses.lower_bound(position) != it.uses.end() and it.reg.is_float == is_float){
                    LOG_DEBUG<<nextUsePos[it.reg]<<" "<<*it.uses.lower_bound(position);
                    nextUsePos[it.reg] = std::min(nextUsePos[it.reg], *it.uses.lower_bound(position));
                    LOG_DEBUG<<it.reg<<" "<<nextUsePos[it.reg];
                }
            if (nextUsePos.empty())
                pos = 0;
            else
            //右端点最大的点 贪心策略
                std::tie(reg, pos) = *std::max_element(
                    nextUsePos.begin(), nextUsePos.end(), [](auto &lhs, auto &rhs) { return lhs.second < rhs.second; });
            LOG_DEBUG << "Reg: " << reg << ", pos: " << pos;
            if (current.uses.empty()) { //如果没使用 直接放到栈上
                // if (not sp_offset.count(current.val)) {  // allocate once
                //     sp_offset[current.val] = stack_size[f_];
                //     stack_size[f_] += current.val->get_type()->get_size();
                // }
                spill(current.val);
                LOG_WARNING << "spill whole current: " << current;
                continue;
            }
            if (*current.uses.begin() >= pos) { 
                if (*current.uses.begin() == pos and *current.uses.begin() == position) { //postion current left point
                    LOG_DEBUG << "too many active interval at " << pos << ", must spill current";
                    if (std::next(current.uses.begin()) == current.uses.end())//活跃一次
                        pos = std::numeric_limits<size_t>::max();
                    else
                        pos = *std::next(current.uses.begin());
                } else
                    // spill current
                    pos = *current.uses.begin();  // save begin of uses, because uses might be changed during `split
                auto split = current.split(pos);
                // if (not sp_offset.count(current.val)) {  // allocate once
                //     sp_offset[current.val] = stack_size[f_];
                //     stack_size[f_] += current.val->get_type()->get_size();
                // }
                spill(current.val);
                if (not split.empty()) {
                    unhandled.insert(split);
                    if (bb_starts.count(pos))
                        LOG_DEBUG << "split at beginning of bb (" << pos << "), not pushing to `split_moves`";
                    else
                        split_moves.push_back({split.val, pos});
                }
            } else {
                // spill reg
                current.reg = reg;
                decltype(active) temp_active{}, temp_unhandled{};
                LOG_DEBUG << "split active interval for reg at position";
                for (auto itit = active.begin(); itit != active.end();) {
                    auto it = *itit;
                    if (it.reg == reg) {
                        itit = active.erase(itit);
                        auto split = it.split(position);
                        if (not it.empty())
                            temp_active.insert(it);
                        temp_unhandled.insert(split);
                        if (bb_starts.count(position))
                            LOG_DEBUG << "split at beginning of bb (" << position << "), not pushing to `split_moves`";
                        else
                            split_moves.push_back({split.val, position});
                        continue;
                    }
                    ++itit;
                }
                active.merge(temp_active);
                unhandled.merge(temp_unhandled);

                // LOG_DEBUG << "split any inactive interval for reg at the end of its lifetime hole";
                for (auto itit = inactive.begin(); itit != inactive.end();) {
                    auto it = *itit;
                    if (it.reg == reg) {
                        itit = inactive.erase(itit);
                        auto j = *it.s.lower_bound(interval_ssa::I::closed(position, position));
                        LOG_DEBUG << j;
                        pos = j.lower();
                        auto split = it.split(pos);
                        if (bb_starts.count(pos))
                            LOG_DEBUG << "split at beginning of bb (" << pos << "), not pushing to `split_moves`";
                        else
                            split_moves.push_back({split.val, pos});
                        LOG_DEBUG << "Split inactive interval " << split.val->get_name() << " at " << pos;
                        // split_moves.push_back({split.val, split.start()});
                        temp_active.insert(it);
                        temp_unhandled.insert(split);
                        continue;
                    }
                    ++itit;
                }
                inactive.merge(temp_active);
                unhandled.merge(temp_unhandled);
            }
        }
        if (current.reg.valid()) {
            active.insert(current);
        }
    }
    for (auto &it : handled) {
        reg_mapping[it.val].push_back(it);
        f_reg_mapping[it.val].push_back(it);
    }
    for (auto &it : active) {
        reg_mapping[it.val].push_back(it);
        f_reg_mapping[it.val].push_back(it);
    }
    for (auto &it : inactive) {
        reg_mapping[it.val].push_back(it);
        f_reg_mapping[it.val].push_back(it);
    }
    for (auto [val, ints] : f_reg_mapping) {
        for (auto &it : ints) {
            for (auto [callp, inst] : call_pos[f_]) {
                if (it.cover(callp - 1, callp) and ((it.reg.id <= int_arg_max_save_index and !it.reg.is_float) or
                                                    (it.reg.id <= fp_arg_max_save_index and it.reg.is_float)))
                    call_saves[inst].insert(it.reg);
            }
        }
    }
    std::string log = "register allocation result\n";
    for (auto [val, ints] : f_reg_mapping) {
        log += val->get_name() + ", " + ints.front().meta + ": ";
        for (auto interval : ints) {
            std::stringstream ss;
            ss << interval.s;
            log += interval.reg.get_name() + " " + ss.str() + ", ";
        }
        if (sp_offset.count(val)) {
            log += "sp(";
            log += std::to_string(sp_offset[val]);
            log += ")";
        }
        log += "\n";
    }
    for (auto [val, offset] : sp_offset) {
        log += val->get_name() + ", ";
        log += "sp(";
        log += std::to_string(offset);
        log += ")";

        log += "\n";
    }
    LOG_INFO << log;
    resolve();
}

/// \brief mov between phi operands and phi instructions
void RegAlloc::resolve() {
    LOG_DEBUG << "resolve";
    for (auto pre : f_->get_basic_blocks()) {
        for (auto suc : pre->get_succ_basic_blocks()) {
            auto livein = liveIn[suc];
            //因为把CFG图dfs成了一个序列 本来是挨着执行的 
            for(auto live:livein){
                LOG_DEBUG<<live.get();;
            }
            auto begin = bb_range[suc].first;  //bb_range记录bb第一条指令到最终指令的interval的左区间端点
            auto pred_end = bb_range[pre.get()].second - 1;  // pred_end is not inclusive, need to minus one? 
            std::set<Value *> phis{};
            for (auto val : suc->get_instructions()){
                if (auto phi = dynamic_cast<PhiInst *>(val.get())) {
                    if (not phi->get_use_list().empty())
                        phis.insert(phi);
                } else
                    break;
            }
            //这些phi都是活跃的
            livein.merge(phis);
            LOG_DEBUG << "pred(" << pre->get_name() << ") end (minus one): " << pred_end << ", succ(" << suc->get_name()
                      << ") begin: " << begin;
            for (auto val : livein) {
                if (val->get_use_list().empty())
                    continue;
                armval move_from;
                // live at beginning of suc
                PhiInst *phi = dynamic_cast<PhiInst *>(val);
                armval move_to;
                if (reg_mapping.count(val)) //val是否占用寄存器
                    LOG_DEBUG << val->get_name() << " starts " << reg_mapping.at(val).front().start();
                else
                    LOG_DEBUG << val->get_name() << " always in memory";
                if (phi and phi->get_parent() == suc) {  // reg_mapping[phi].front().start() == begin + 1 
                    move_to = get_loc(val, begin);
                    //phi指令其中一个前驱是已经定义了的
                    auto op = phi->input_of(pre.get());
                    if (not op)  // undef
                        continue;
                    if (isa<Constant>(op.get()) or isa<AllocaInst>(op.get())) {
                        // %output = phi [1, bb]
                        if (auto ci = dynamic_cast<ConstantInt *>(op.get())) {
                            LOG_DEBUG << "move const int " << ci->get_value() << " to phi";
                            move_from = ci;
                        } else if (auto cf = dynamic_cast<ConstantFP *>(op.get())) {
                            LOG_DEBUG << "move const float " << cf->get_value() << " to phi";
                            move_from = cf;
                        } else if (auto alloca = dynamic_cast<AllocaInst *>(op.get())) {
                            LOG_DEBUG << "move alloca " << alloca->get_name() << " to phi";
                            move_from = alloca;
                        }
                    } else {
                        LOG_DEBUG << "move value " << op->get_name() << " to phi";
                        move_from = get_loc(op.get(), pred_end);
                    }
                } else {
                    move_to = get_loc(val, begin);
                    // not phi, but its interval is split during `build_intervals`
                    move_from = get_loc(val, pred_end);
                }
                if (move_from != move_to) {
                    LOG_DEBUG << "resolve " << val->get_name();
                
                    move_mapping[{pre.get(), suc}].push_back({move_to, move_from});
                }
            }

            for (auto [move_to, move_from] : move_mapping[{pre.get(), suc}]) {
                LOG_INFO << "move from: " << move_from << ", move to: " << move_to;
            }
        }
    }
    //
    for (auto &[val, pos] : split_moves) {
        armval pre = get_loc(val, pos - 1);
        bool pre_reg = std::holds_alternative<Reg>(pre);
        armval succ = get_loc(val, pos);
        if (pre != succ)
            ordered_moves[pos].push_back({succ, pre});  // mov to, from
        bool succ_reg = std::holds_alternative<Reg>(succ);
        LOG_DEBUG << "Resolving " << val->get_name() << " at position " << pos;
        if (pre_reg and succ_reg) {
            LOG_DEBUG << "move between registers " << pre << " and " << succ;
            if (pre != succ)
                moves[pos].push_back({get(val, pos - 1), get(val, pos)});
        } else if (not pre_reg and succ_reg) {
            LOG_DEBUG << "Load to register " << succ;
            loads[pos].push_back({sp_offset.at(val), get(val, pos)});
        } else if (pre_reg and not succ_reg) {
            LOG_DEBUG << "Store " << pre << " to memory ";
            stores[pos].push_back({get(val, pos - 1), sp_offset.at(val)});
        } else
            LOG_DEBUG << "Both in memory";
    }
    f_split_moves[f_] = move(split_moves);
    fstores[f_] = move(stores);
    floads[f_] = move(loads);
    fmoves[f_] = move(moves);
    fordered_moves[f_] = move(ordered_moves);
}

/// \returns only a Reg or a size_t
armval RegAlloc::get_loc(Value *val, size_t index) {
    if (not reg_mapping.count(val)) {
        if (not val->get_type()->is_float_type())
            return armval{std::in_place_index<int_variant_index>, sp_offset.at(val)};
        return armval{std::in_place_index<float_variant_index>, sp_offset.at(val)};
    }
    Reg r = get(val, index);
    if (r.valid())
        return r;
    if (not val->get_type()->is_float_type())
        return armval{std::in_place_index<int_variant_index>, sp_offset.at(val)};
    return armval{std::in_place_index<float_variant_index>, sp_offset.at(val)};
}

Reg RegAlloc::get(Value *val, size_t index) {
    const std::vector<interval_ssa> &intervals = reg_mapping.at(val);
    for (auto &interval : intervals) {
        if (interval.cover(index))
            return interval.reg;
    }
    if (not sp_offset.count(val)) {
        
        LOG_ERROR << val->get_name() << " not valid at index " << index;
        exit(212);
    }
    else
        LOG_WARNING << val->get_name() << " might be spilled at index " << index << ", check .valid()";
    // exit(121);
    return Reg();
}

void RegAlloc::log_intervals() {
    std::string log_reg = "interval for virtual registers\n";
    for (auto [k, v] : ssa_intervals) {
        log_reg += k->get_name() + ", " + v.meta + ": ";
        std::stringstream ss;
        ss << v.s << " uses: [";
        for (auto p : v.uses)
            ss << p << ",";
        ss << "]";
        log_reg += ss.str();
        log_reg += "\n";
    }
    LOG_INFO << log_reg;
}

void RegAlloc::run() {
    LOG_DEBUG << "Running register allocation";
    loops = std::make_unique<loop_search>(m_);
    loops->run();
    for (auto f : m_->get_functions()) {
        if (f->get_num_basic_blocks() == 0)
            continue;
        f_ = f.get();
        stack_size[f_] = 16;
        init_func();
        // assert(f->get_num_basic_blocks() == linear_list.size());
        f->get_basic_blocks() = move(linear_list);
        build_intervals();
        check_intervals();
        log_intervals();
        linear_scan_ssa();
    }
}

void RegAlloc::check_intervals() {
    for (auto [k, v] : ssa_intervals) {
        assert(k and k == v.val);
        assert(not v.s.empty());
        // if (!(std::all_of(v.uses.begin(), v.uses.end(), [](auto n) { return n % 2 == 0; }))) exit(19);
    }
}

// FIXME: dfs is not enough, need to make sure all blocks in a loop are contiguous
// try construct a linear order on dominant tree
void RegAlloc::dfs(BasicBlock *s) {
    if (visited[s])
        return;
    visited[s] = 1;

    auto base2loop = loops->get_base2loop();
    bb_set_t loopset{};
    if (base2loop.count(s)) {   //如果s是循环的起始块
        loopset = *base2loop.at(s);  //取对应的loop_bb_set
        loopset.erase(s); //删除头节点
    }
    auto &sucbbs = s->get_succ_basic_blocks(); //当前s的后继bb
    for (auto it = loopset.begin(); it != loopset.end();) {  // we assume all the visited nodes in a loop are contiguous
        if (visited[*it] or std::find(sucbbs.begin(), sucbbs.end(), *it) == sucbbs.end())  //loop_bb中bb已经被访问过或者其不在s的后继bb中
            it = loopset.erase(it);
        else
            it++;
    }
    for (auto suc : loopset) {
        if (!visited[suc])
            dfs(suc);
    }
    for (auto suc : s->get_succ_basic_blocks()) {
        if (!visited[suc])
            dfs(suc);
    }
    visited[s] = 2;
    inst_count += 2 * s->get_num_inst();  // multiply by 2 to distinguish inputs and output
}

// FIXME: sccp会产生不可达块，run with cfg simplification
void RegAlloc::construct_linear_order() {
    linear_set.clear();
    linear_list.clear();
    const auto &base2loop = loops->get_base2loop();
    std::set<BasicBlock *> bbs;
    auto entry = f_->get_entry_block().get();
    bbs.insert(entry);
    while (not bbs.empty()) {
        BasicBlock *current{};
        for (auto bb : bbs) {
            // find a bb whose predecessors (except back edges) are already in list
            //(bb的所有前驱都在linear_set||bb的前驱为循环内容)&&bb为循环入口
            if (auto &pred_of_bb = bb->get_pre_basic_blocks();
                base2loop.count(bb) and std::all_of(pred_of_bb.begin(), pred_of_bb.end(), [&, this](auto pred) {
                    return linear_set.count(pred) or base2loop.at(bb)->count(pred);
                })) {
                // if all pred other than loop elements are in linear_set
                current = bb;
                break;
            }//非循环体头但满足所有前驱都在linear_set中 
            else if (auto &pred_of_bb = bb->get_pre_basic_blocks();
                       std::all_of(pred_of_bb.begin(), pred_of_bb.end(), [&, this](auto pred) {
                           return linear_set.count(pred);
                       })) {
                current = bb;  // keep looking in case there is a loop header
            }
        }
        LOG_DEBUG<<current->print();
        assert(current and "must be non-null");
        //如果为循环头
        if (base2loop.count(current)) {
            //current循环体里的所有BB
            auto &loopset = *base2loop.at(current);
            for(auto i:loopset){
                LOG_DEBUG<<"base: "<<current->get_name()<<" "<<i->get_name();

            }
            construct_linear_order_recur(loopset, current);
            for (auto loop_block : loopset) {
                bbs.erase(loop_block);
                for (auto succ : loop_block->get_succ_basic_blocks())
                    if (not loopset.count(succ) and not linear_set.count(succ))
                        bbs.insert(succ);
            }
        } else {
            linear_set.insert(current);
            linear_list.push_back(std::dynamic_pointer_cast<BasicBlock>(current->shared_from_this()));
            assert(linear_set.size() == linear_list.size());
            bbs.erase(current);

            auto &succs = current->get_succ_basic_blocks();
            for (auto succ : succs)
                bbs.insert(succ);
        }
    }
    LOG_DEBUG << "final linear bbs: ";
    for (auto bb : linear_list)
        LOG_DEBUG << bb->get_name();
}

void RegAlloc::construct_linear_order_recur(const bb_set_t &loopset, BasicBlock *entry) {
    const auto &base2loop = loops->get_base2loop();
    std::set<BasicBlock *> bbs;
    bbs.insert(entry);
    while (not bbs.empty()) {
        BasicBlock *current;
        for (auto bb : bbs) {
            // find a bb whose predecessors (except back edges) are already in list
            if (auto &pred_of_bb = bb->get_pre_basic_blocks();
                base2loop.count(bb) and std::all_of(pred_of_bb.begin(), pred_of_bb.end(), [&, this](auto pred) {
                    return linear_set.count(pred) or base2loop.at(bb)->count(pred);
                })) {
                // if all pred other than loop elements are in linear_set
                current = bb;
                break;
            } else if (auto &pred_of_bb = bb->get_pre_basic_blocks();
                       std::all_of(pred_of_bb.begin(), pred_of_bb.end(), [&, this](auto pred) {
                           return linear_set.count(pred);
                       })) {
                current = bb;  // keep looking in case there is a loop header
            }
        }
        assert(current and "must be non-null");
        if (base2loop.count(current) and current != entry) {
          
            auto &inner_loop = *base2loop.at(current);
              for(auto i:inner_loop){
                LOG_DEBUG<<"base: "<<current->get_name()<<" "<<i->get_name();

            }
            construct_linear_order_recur(inner_loop, current);
            for (auto loop_block : inner_loop) {
                bbs.erase(loop_block);
                for (auto succ : loop_block->get_succ_basic_blocks())
                    if (loopset.count(succ) and not linear_set.count(succ))
                        bbs.insert(succ);
            }
        } else {
            linear_set.insert(current);
            linear_list.push_back(std::dynamic_pointer_cast<BasicBlock>(current->shared_from_this()));
            bbs.erase(current);

            auto &succs = current->get_succ_basic_blocks();
            for (auto succ : succs)
                if (loopset.count(succ) and not linear_set.count(succ))  // only process the loop
                    bbs.insert(succ);
        }
    }
    LOG_DEBUG << "linear bbs: ";
    for (auto bb : linear_list)
        LOG_DEBUG << bb->get_name();
}

void RegAlloc::init_func() {
    visited.clear();
    inst_count = 0;
    bb_index = f_->get_num_basic_blocks();
    ssa_intervals.clear();
    split_moves.clear();
    stores.clear();
    loads.clear();
    moves.clear();
    dfs(f_->get_entry_block().get());
    construct_linear_order();
    for (auto i = 0; i < 5; i++) {
        freeUntilPositionInteger[Reg(i)] = 0;
        nextUsePosInteger[Reg(i)] = 0;
    }
    for (auto i = 0; i < 32 - 2; i++) {
        freeUntilPositionFloat[Reg(i, true)] = 0;
        nextUsePosFloat[Reg(i, true)] = 0;
    }
}

std::ostream &operator<<(std::ostream &os, const armval &val) {
    return std::visit(
        [&](const auto &p) -> std::ostream & {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, Reg>)
                return os << p;
            else if constexpr (std::is_same_v<T, Value *>)
                return os << "const " << p->print();
            else if constexpr (std::is_same_v<T, size_t>)
                return os << "sp(" << p << ")";
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
        },
        val);
}

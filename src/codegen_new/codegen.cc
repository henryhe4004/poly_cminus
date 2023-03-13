#include "codegen.hh"

#include "lir.h"
#include "regalloc.hh"
#include "utils.hh"

#include <boost/hana/functional/fix.hpp>
#include <map>
#undef NDEBUG
const Reg Codegen::temp_lhs_reg(19), Codegen::temp_rhs_reg(20), Codegen::temp_out_reg(21);
const Reg Codegen::temp_float_lhs_reg(30, true), Codegen::temp_float_rhs_reg(31, true);

using std::map;
using std::string;

void gen_fcmp_move(FCmpInst *fcmp_inst, Reg target_Reg);

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// abi:
// https://developer.arm.com/documentation/den0013/d/Application-Binary-Interfaces/Procedure-Call-Standard

using std::string_view_literals::operator""sv;

string get_label_name(BasicBlock *bb) {
    auto func = bb->get_parent();
    return "." + func->get_name() + "_" + bb->get_name();
}

map<ConstantFP *, int> constFloat_to_index;
std::string func_name;

// string get_label_name_float(ConstantFP *c) {
//     // .main_const:
//     //      .word 0x3f800000
//     LOG_DEBUG << c->get_value();
//     if (constFloat_to_index.find(c) == constFloat_to_index.end()) {
//         int count = constFloat_to_index.size();
//         constFloat_to_index[c] = count;
//     }
//     return "." + func_name + "_const+" + std::to_string(constFloat_to_index[c] * 4);
// }

void Codegen::run() {
    RegAlloc ra(m, reg_num);
    ra.run();

    // // 更新 reg.id 为从 4 开始的编号
    // for (auto [val, intervals] : ra.reg_mapping)
    //     for (auto interval : intervals)
    //         // interval.reg.id += 4;
    //         interval.reg = Reg(interval.reg.id + 4);

    this->reg_mapping = std::move(ra.reg_mapping);
    this->move_mapping = std::move(ra.move_mapping);
    this->sp_offset = std::move(ra.sp_offset);
    this->stack_size = std::move(ra.stack_size);
    this->call_saves = std::move(ra.call_saves);
    // generate global variables
    gen_got();
    // output << "\t.fpu vfpv3-d16\n";
    // output << "\t.cpu cortex-a15\n";
    // output << "\t.section .note.GNU-stack,\"\",%progbits\n";
    output << "\t.text\n";
    for (auto f : m->get_functions())
        if (not f->is_declaration()) {
            Function *f_ = f.get();
            this->moves = std::move(ra.fmoves[f_]);
            this->stores = std::move(ra.fstores[f_]);
            this->loads = std::move(ra.floads[f_]);
            this->ordered_moves = std::move(ra.fordered_moves[f_]);
            gen_function(f_);
        }
}

void Codegen::gen_got() {
    std::string asm_code;
    this->GOT.clear();
    for (auto &gl : m->get_global_variables()) {
        auto global_var = gl.get();
        int count = this->GOT.size();
        if (!GOT.count(global_var)) {
            this->GOT[global_var] = count;
        }
    }

    std::vector<GlobalVariable *> vecGOT;

    vecGOT.resize(this->GOT.size());
    for (auto &i : GOT) {
        vecGOT[i.second] = i.first;
    }

    output << "\t.data\n";
    std::for_each(vecGOT.begin(), vecGOT.end(), [&](GlobalVariable *gl) {
        if (gl->get_init()->is_constant_zero()) {
            output << "\t.comm\t" << gl->get_name() << "," << gl->get_init()->get_type()->get_size() << "\n";
            return;
        }
        output << gl->get_name() << ":\n";
        auto size = gl->get_type()->get_size();
        size_t allocated = 0;
        boost::hana::fix([&](auto print_init, Constant *ci) -> void {
            std::string init;
            if (ci->is_constant_zero()) {
                // output << "\t.word " << 0 << " # " << (allocated += 4) << "\n"; // do nothing
                auto zero_init = dynamic_cast<ConstantZero *>(ci);
                output << "\t.space " << zero_init->get_type()->get_size() << "\n";
                allocated += zero_init->get_type()->get_size();
            } else if (ci->is_constant_int()) {
                output << "\t.word " << dynamic_cast<ConstantInt *>(ci)->get_value() << " # " << (allocated += 4)
                       << "\n";
            } else if (ci->is_constant_array()) {
                auto cai = dynamic_cast<ConstantArray *>(ci);
                for (auto ci : cai->get_value())
                    print_init(ci.get());
            } else if (ci->is_constant_fp()) {
                auto cfi = dynamic_cast<ConstantFP *>(ci);
                auto float_value = cfi->get_value();
                auto int_value = *(int *)&float_value;
                output << "\t.word " << int_value << " # " << allocated << "\n";
                allocated += 4; 
            } else if (ci->is_constant_struct()) {
                auto c_struct = dynamic_cast<ConstantStruct *>(ci);
                for (auto ci : c_struct->get_elements())
                    print_init(ci.get());
            } else
                LOG_ERROR << "unsupported initialization type";
        })(gl->get_init().get());
        if (allocated < size) {
            output << "\t.space " << size - allocated << "\n";
        }
    });
}

void Codegen::gen_function(Function *f) {
    /*
    .globl	main
    .type	main, @function                 # @test
    */
    output << "\t.globl\t" << f->get_name() << "\n";
    // output << "\t.p2align\t2\n";
    output << "\t.type\t" << f->get_name() << ", @function\n";
    // output << "\t.code\t32\n";
    output << f->get_name() << ":\n";
    prologue(f); // 序言 执行fp指向sp sp向下增长的内容

    pos = 0;
    caller_saved.clear();

    int load_index_float = 0, load_index_int = 0;
    std::vector<Value *> load_on_stack;
    //参数放入寄存器中 放不下就放到栈上
    for (auto arg_share : f->get_args()) {
        auto arg = arg_share.get();
        auto is_float = arg->get_type()->is_float_type();
        //fp_arg_max_save_index = 15 int_arg_max_save_index =16 
        int max_save_index = is_float ? RegAlloc::fp_arg_max_save_index : RegAlloc::int_arg_max_save_index;
        
        int &save_index = is_float ? load_index_float : load_index_int;
        if (save_index > max_save_index)
            load_on_stack.push_back(arg);
        else
            save_index++;
    }
    //如果参数大于寄存器 放入内存中
    LOG_DEBUG<<load_on_stack.size();
    for (size_t i = 0; i < load_on_stack.size(); i++) {
        auto arg = load_on_stack[i];
        LOG_DEBUG << "Load argument " << arg->get_name() << " from stack";

        if (callee_saved_size > 0)
            LOG_ERROR << "callee_saved_size = " << callee_saved_size;

        if (not arg->get_use_list().empty()) {
            if (reg_mapping.count(arg) and get(arg).valid()) {
                Reg reg = get(arg);
                if (not reg.is_float)
                    Instgen::ld_d(get(arg), Reg::ID::fp, i * 8 + callee_saved_size * 4);  // 需要更改 * 4
                else
                    Instgen::fld_s(get(arg), Reg::ID::fp, i * 8 + callee_saved_size * 4);
            } else {
                Instgen::ld_d(temp_lhs_reg, Reg::ID::fp, i * 8 + callee_saved_size * 4);
                Instgen::st_d(temp_lhs_reg, Reg::ID::sp, sp_offset.at(arg));
            }
        }
    }
    //生成basic block
    for (auto bb : f->get_basic_blocks())
        gen_bb(bb.get());
    epilogue(f); //恢复原来的fp与sp
    output << "\n";
}

#define rotate_left(v, n) (v << n | v >> (32 - n))

bool encode_arm_immediate(unsigned int val) {
    unsigned int a, i;

    for (i = 0; i < 32; i += 2)
        if ((a = rotate_left(val, i)) <= 0xff)
            return true; /* 12-bit pack: [shift-cnt,const].  */

    return false;
}

void Codegen::prologue(Function *f) {
    // should push callee-saved registers that are allocated
    // r11 is used as frame pointer
    // but armv7 does not specify a frame pointer and we don't actually need it
    // Instgen::push({Reg(11), Reg::ID::lr});

    // 最好保证 sp 为 8 的倍数，否则浮点操作可能出错
    // output << "\tpush\t{r4-r11, lr}\n";  // temp
    // output << "\tvpush\t{d8-d15}\n";     // temp
    // Instgen::sub(Reg::ID::sp, Reg::ID::sp, 4);
    // callee_saved_size = (11 - 4 + 2) + 2 * (15 - 8 + 1) + 1;

    // func_name = f->get_name();
    // constFloat_to_index.clear();

    // Instgen::mov(Reg(11), Reg::ID::sp);
    auto size = stack_size.at(f);
    LOG_DEBUG << "stack size: " << size;
    if (size % 8 != 0)
        size += 4;

    Instgen::mov(Reg::ID::fp, Reg::ID::sp);

    if (size < 2048)
        Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, -size);
    else {
        Instgen::mov(temp_lhs_reg, size);
        Instgen::sub_d(Reg::ID::sp, Reg::ID::sp, temp_lhs_reg);
    }
    Instgen::st_d(Reg::ID::ra, Reg::ID::sp, 8);  // 不同于 gcc 的结果的栈布局，gcc 在 size - 8 处存储 ra

    // 	addi.d	$r3,$r3,-16
    //  st.d	$r22,$r3,8
    // addi.d	$r22,$r3,16

    // output << "\t.cfi_startproc\n";
    // output << "\t.cfi_def_cfa_offset " << size << "\n";
    // output << "\t.cfi_offset " << 1 << ", " << 8 << "\n";
}

void Codegen::epilogue(Function *f) {
    auto size = stack_size.at(f);
    if (size % 8 != 0)
        size += 4;
    // if (encode_arm_immediate(size))
    //     Instgen::add(Reg::ID::sp, Reg::ID::sp, size);
    // else {
    //     move(temp_lhs_reg, size);
    //     Instgen::add(Reg::ID::sp, Reg::ID::sp, temp_lhs_reg);
    // }
    // Instgen::add(Reg::ID::sp, Reg::ID::sp, stack_size.at(f));
    // Instgen::pop({Reg(11), Reg::ID::pc});
    // output << "\tvpop\t{d8-d15}\n";  // temp
    // output << "\tpop\t{r4-r11, pc}\n";

    // if (constFloat_to_index.size() > 0) {
    //     output << "." << func_name << "_const:\n";
    //     vector<ConstantFP *> constFloats(constFloat_to_index.size());
    //     for (auto [value, index] : constFloat_to_index)
    //         constFloats[index] = value;
    //     for (auto c : constFloats) {
    //         auto floatValue = c->get_value();
    //         auto intValue = *(int *)&floatValue;
    //         output << "\t.word " << intValue << "\n";
    //     }
    // }

    // 	ld.d	$r22,$r3,8
    // addi.d	$r3,$r3,16
    // jr	$r1
    Instgen::ld_d(Reg::ID::ra, Reg::ID::sp, 8);

    if (size < 2048)
        Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, size);
    else {
        Instgen::mov(temp_lhs_reg, size);
        Instgen::add_d(Reg::ID::sp, Reg::ID::sp, temp_lhs_reg);
    }
    Instgen::jr(Reg::ID::ra);
    // output << "\t.cfi_endproc\n";
    output << "\n";
}

void Codegen::gen_bb(BasicBlock *bb) {
    LOG_DEBUG<<""<<get_label_name(bb);
    output << get_label_name(bb) << ":\n";
    for (auto inst : bb->get_instructions()) {
        auto i = inst.get();
        resolve_split(pos);
        if (not inst->is_br())  // if branch inst, resolve in it
            resolve_split(pos + 1);
        if (not inst->is_call() and (not inst->is_void() and inst->get_use_list().empty())) {
            output << "\t# " << pos << " ignored\n";
            pos += 2;
            continue;
        }
        output << "\t# " << pos << " " << inst->print() << "\n";
        LOG_DEBUG<<inst->print();
        //具体每条指令的asm生成
        switch (inst->get_instr_type()) {
            case Instruction::alloca:  // already allocated
                break;
            case Instruction::add:
            case Instruction::sub:
            case Instruction::rsub:
            case Instruction::mul:
            case Instruction::ashr:
            case Instruction::and_:
            case Instruction::lshr:
            case Instruction::shl:
            case Instruction::sdiv:
            case Instruction::srem:
            case Instruction::fadd:
            case Instruction::fsub:
            case Instruction::fmul:
            case Instruction::fdiv:
                gen_binary(dynamic_cast<BinaryInst *>(i));
                break;
            case Instruction::cmp:
                gen_cmp(dynamic_cast<CmpInst *>(i));
                break;
            case Instruction::fcmp:
                // 在 gen_br 时会调用 gen_cmp (no!)
                gen_fcmp(dynamic_cast<FCmpInst *>(i));
                break;
            case Instruction::select:
                gen_select(dynamic_cast<SelectInst *>(i));
                break;
            case Instruction::zext:
                gen_zext(dynamic_cast<ZextInst *>(i));
                break;
            case Instruction::fptosi:
                gen_fptosi(dynamic_cast<FpToSiInst *>(i));
                break;
            case Instruction::sitofp:
                gen_sitofp(dynamic_cast<SiToFpInst *>(i));
                break;
            case Instruction::call:
                gen_call(dynamic_cast<CallInst *>(i));
                break;
            case Instruction::phi:
                // resolved during call to gen_br
                break;
            case Instruction::br:
                gen_br(bb, dynamic_cast<BranchInst *>(i));
                break;
            case Instruction::ret:
                gen_ret(dynamic_cast<ReturnInst *>(i));
                break;
            // case Instruction::switch1:
            //     // TODO
            //     gen_switch(bb, dynamic_cast<SwitchInst *>(i));
            //     break;
            case Instruction::mov:
                move(inst->get_operand(0).get(), inst->get_operand(1).get());
                pos -= 2;  // mov is not allocated register
                break;
            case Instruction::inttoptr:
                // no need for additional operations, just move values
                // TODO: check if `inst` has a const offset to an alloca so we can save one register
                move(i, inst->get_operand(0).get());
                break;
            case Instruction::ptrtoint:
                // unlike inttoptr, we might have an alloca, which is not allocated memory (or regs)
                move(i, inst->get_operand(0).get());
                break;
            case Instruction::store:
                store(inst->get_operand(0).get(), inst->get_operand(1).get(), dynamic_cast<StoreInst *>(i));
                break;
            case Instruction::load:
                load(i, inst->get_operand(0).get(), dynamic_cast<LoadInst *>(i));
                break;
            case Instruction::getelementptr:
                LOG_ERROR<<"getelementptr:";
                LOG_ERROR << "Please enable '-gep-elim'";
                exit(209);
                break;
            case Instruction::muladd:
            case Instruction::mulsub:
                gen_ternary(i);
                break;
            default:
                LOG_ERROR << "Unknown instruction: " << inst->print();
                exit(210);
                break;
        }
        pos += 2;
    }
}

void Codegen::resolve_split(size_t pos, bool store, bool load, bool move) {

    if (ordered_moves.count(pos)){
        LOG_DEBUG<<"ordered_moves.count(pos)";
        parallel_mov(ordered_moves[pos]);
    }
}

void Codegen::get_reg_and_move(Reg &r, Value *v) {
    if (reg_mapping.count(v) and get(v).valid())
        r = get(v);
    move(r, v);
}

void Codegen::gen_ternary(Instruction *inst) {
    auto op1 = inst->get_operand(0).get();
    auto op2 = inst->get_operand(1).get();
    auto op3 = inst->get_operand(2).get();
    Reg op1_reg = temp_lhs_reg;
    Reg op2_reg = temp_rhs_reg;
    Reg op3_reg = temp_out_reg;
    Reg dest_reg = get(inst, true);
    get_reg_and_move(op1_reg, op1);
    get_reg_and_move(op2_reg, op2);
    get_reg_and_move(op3_reg, op3);
    if (dynamic_cast<MuladdInst *>(inst)) {
        Instgen::mla(dest_reg, op1_reg, op2_reg, op3_reg);
    } else if (dynamic_cast<MulsubInst *>(inst)) {
        Instgen::mls(dest_reg, op1_reg, op2_reg, op3_reg);
    } else
        LOG_ERROR << "Unsupported ternary instruction";
}

void Codegen::gen_binary(BinaryInst *inst) {
    auto lhs = inst->get_operand(0).get();
    auto rhs = inst->get_operand(1).get();
    //浮点数加减乘除
    if (inst->is_fadd() or inst->is_fsub() or inst->is_fmul() or inst->is_fdiv()) {
        Reg lhs_reg = temp_float_lhs_reg;
        Reg rhs_reg = temp_float_rhs_reg;

        auto lhs_const = dynamic_cast<ConstantFP *>(lhs);
        auto rhs_const = dynamic_cast<ConstantFP *>(rhs);

        if (not lhs_const)
            lhs_reg = get(lhs);
        if (not rhs_const)
            rhs_reg = get(rhs);

        move(lhs_reg, lhs);
        move(rhs_reg, rhs);
        Reg dest_float_reg = get(inst, true);

        if (inst->is_fadd())
            Instgen::fadd_s(dest_float_reg, lhs_reg, rhs_reg);
        else if (inst->is_fsub())
            Instgen::fsub_s(dest_float_reg, lhs_reg, rhs_reg);
        else if (inst->is_fmul())
            Instgen::fmul_s(dest_float_reg, lhs_reg, rhs_reg);
        else if (inst->is_fdiv())
            Instgen::fdiv_s(dest_float_reg, lhs_reg, rhs_reg);
    } else {
        Reg lhs_reg = temp_lhs_reg;
        Reg rhs_reg = temp_rhs_reg;
        std::string_view set_flag = inst->set() ? "s" : "";
        auto lhs_no_reg = isa<ConstantInt>(lhs) or isa<AllocaInst>(lhs);
        auto rhs_no_reg = isa<ConstantInt>(rhs) or isa<AllocaInst>(rhs);

        if (isa<ConstantInt>(rhs)) {
            int rhs_val = dynamic_cast<ConstantInt *>(rhs)->get_value();
            if (inst->is_and_() or inst->is_shl() or inst->is_ashr() or inst->is_lshr()) {
                if (not lhs_no_reg and get(lhs).valid())
                    lhs_reg = get(lhs);
                move(lhs_reg, lhs);
                Reg dest_reg = get(inst, true).valid() ? get(inst, true) : Reg(11);
                if (inst->is_and_())
                    Instgen::andi(dest_reg, lhs_reg, rhs_val);
                else if (inst->is_shl())
                    Instgen::slli_w(dest_reg, lhs_reg, rhs_val);
                else if (inst->is_ashr())
                    Instgen::srai_w(dest_reg, lhs_reg, rhs_val);
                else if (inst->is_lshr())
                    Instgen::srli_w(dest_reg, lhs_reg, rhs_val);
                return;
            }
        }

        if (not lhs_no_reg and get(lhs).valid())
            lhs_reg = get(lhs);
        if (not rhs_no_reg and get(rhs).valid())
            rhs_reg = get(rhs);

        move(lhs_reg, lhs);
        move(rhs_reg, rhs);
        rhs_reg.op = inst->get_op2_shift_type();
        rhs_reg.shift_n = inst->get_op2_shift_bits();

        Reg dest_reg = get(inst, true).valid() ? get(inst, true) : Reg(11);  // ?
        if (inst->is_add())
            Instgen::add_d(dest_reg, lhs_reg, rhs_reg);
        else if (inst->is_sub())
            Instgen::sub_d(dest_reg, lhs_reg, rhs_reg);
        else if (inst->is_mul())
            Instgen::mul_w(dest_reg, lhs_reg, rhs_reg);
        else if (inst->is_div())
            Instgen::div_w(dest_reg, lhs_reg, rhs_reg);
        else if (inst->is_srem())
            Instgen::mod_w(dest_reg, lhs_reg, rhs_reg);
        else{
            LOG_DEBUG<<"error";
            exit(ABNORMAL_ERROR, "unknown binary instruction");
        }
    }
}
void Codegen::gen_select(SelectInst *sel) {
    LOG_DEBUG<<"select";
    Reg target_reg = get(sel, true);
    auto lhs = sel->get_operand(1).get();
    auto rhs = sel->get_operand(2).get();
    auto test_bit = sel->get_operand(0).get();
    if (reg_mapping.count(test_bit)) {
        //maskeqz必须为寄存器 得判断是否为立即数
        Reg lhs_reg = temp_lhs_reg;
        Reg rhs_reg = temp_rhs_reg;
        auto cond_reg = get(test_bit);
        auto lhs_const = dynamic_cast<ConstantInt *>(lhs);
        auto rhs_const = dynamic_cast<ConstantInt *>(rhs);
        if (not lhs_const)
            lhs_reg = get(lhs);
        if (not rhs_const)
            rhs_reg = get(rhs);
        move(lhs_reg, lhs);
        move(rhs_reg, rhs);
       
        //maskeqz是和0判断 所以相同及不满足条件取右值
        Instgen::maskeqz(target_reg,rhs_reg,cond_reg);
        Instgen::masknez(target_reg,lhs_reg,cond_reg);
    } else
        LOG_ERROR << "no reg";
}

void Codegen::gen_cmp(CmpInst *inst) {
    auto lhs = inst->get_operand(0).get();
    auto rhs = inst->get_operand(1).get();
    Reg lhs_reg, rhs_reg;
    if (reg_mapping.count(lhs) and get(lhs).valid())
        lhs_reg = get(lhs);
    else {  // constants, or variables in stack
        lhs_reg = temp_lhs_reg;
        move(temp_lhs_reg, lhs);
    }
    if (reg_mapping.count(rhs) and get(rhs).valid())
        rhs_reg = get(rhs);
    else {
        rhs_reg = temp_rhs_reg;
        move(temp_rhs_reg, rhs);
    }
    // BasicBlock *parent = inst->get_parent();
    // 或者在寄存器分配的时候，如果无需保存，就不分配了
    // if (reg_mapping.count(inst)) {
    // LOG_DEBUG << "must save this compare result";
    Reg target_Reg = temp_rhs_reg;

    // <    slt l,r     1 represents l < r
    // >    slt r,l     1 represents l > r
    // <=   slt r,l     0 represents l <= r
    // >=   slt l,r     0 represents l >= r
    // ==   sub l,r     0 represents l == r
    // !=   sub l,r     1 represents l != r
    switch (inst->get_cmp_op()) {
        case CmpOp::LT:
            Instgen::slt(target_Reg, lhs_reg, rhs_reg);
            break;
        case CmpOp::GE:
            Instgen::slt(target_Reg, lhs_reg, rhs_reg);
            break;
        case CmpOp::GT:
            Instgen::slt(target_Reg, rhs_reg, lhs_reg);
            break;
        case CmpOp::LE:
            Instgen::slt(target_Reg, rhs_reg, lhs_reg);
            break;
        case CmpOp::EQ:
             Instgen::sub_w(target_Reg, lhs_reg, rhs_reg);
            break;
        case CmpOp::NE:
            Instgen::sub_w(target_Reg, lhs_reg, rhs_reg);
            break;
        default:{
            LOG_ERROR<<"abnormal_error";
            exit(ABNORMAL_ERROR, "unknown compare instruction");
        }
    }
    // }
}

void Codegen::gen_fcmp(FCmpInst *inst) {
    auto lhs = inst->get_operand(0).get();
    auto rhs = inst->get_operand(1).get();
    Reg lhs_reg, rhs_reg;
    if (reg_mapping.count(lhs) and get(lhs).valid())
        lhs_reg = get(lhs);
    else {  // constants, or variables in stack
        lhs_reg = temp_float_lhs_reg;
        move(temp_float_lhs_reg, lhs);
    }
    if (reg_mapping.count(rhs) and get(rhs).valid())
        rhs_reg = get(rhs);
    else {
        rhs_reg = temp_float_rhs_reg;
        move(temp_float_rhs_reg, rhs);
    }

    // fcmp_slt_s
    // fcmp_sune_s
    switch (inst->get_cmp_op()) {
        case CmpOp::LT:
        case CmpOp::GE:
            Instgen::fcmp_slt_s("$fcc0", lhs_reg, rhs_reg);
            break;
        case CmpOp::GT:
        case CmpOp::LE:
            Instgen::fcmp_slt_s("$fcc0", rhs_reg, lhs_reg);
            break;
        case CmpOp::EQ:
        case CmpOp::NE:
            Instgen::fcmp_sune_s("$fcc0", lhs_reg, rhs_reg);
            break;
    }

    if (reg_mapping.count(inst))
        gen_fcmp_move(inst, get(inst, true));
}

void gen_cmp_move(CmpInst *cmp_inst, Reg target_Reg) {
    // 将比较结果写入寄存器
    switch (cmp_inst->get_cmp_op()) {
        case CmpOp::LT:
        case CmpOp::GT:
        case CmpOp::NE:
            Instgen::mov(target_Reg, Codegen::temp_rhs_reg);
            break;
        case CmpOp::LE:
        case CmpOp::GE:
        case CmpOp::EQ:
            Instgen::xori(target_Reg, Codegen::temp_rhs_reg, 1);
            break;
    }
}

void gen_fcmp_move(FCmpInst *fcmp_inst, Reg target_Reg) {
    // Reg rhs_reg;
    // if(reg_mapping.count(fcmp_inst) and get(fcmp_inst).valid()){
    //     rhs_reg = get(fcmp_inst);
    // }else{
    //     //target_Reg 有没有可能就要保存到temp_rhs_reg?
    //     rhs_reg = temp_rhs_reg;
    // }
    // <    slt l,r     1 represents l < r
    // >    slt r,l     1 represents l > r
    // <=   slt r,l     0 represents l <= r
    // >=   slt l,r     0 represents l >= r
    // ==   sub l,r     0 represents l == r
    // !=   sub l,r     1 represents l != r
    switch (fcmp_inst->get_cmp_op()) {
        case CmpOp::LT:
            Instgen::ite("mi");
            
            Instgen::movmi(target_Reg, 1);
            Instgen::movpl(target_Reg, 0);
            break;
        case CmpOp::GT:
            Instgen::ite("gt");
            Instgen::movgt(target_Reg, 1);
            Instgen::movle(target_Reg, 0);
            break;
        case CmpOp::LE:
            Instgen::ite("ls");
            Instgen::movls(target_Reg, 1);
            Instgen::movhi(target_Reg, 0);
            break;
        case CmpOp::GE:
            Instgen::ite("ge");
            Instgen::movge(target_Reg, 1);
            Instgen::movlt(target_Reg, 0);
            break;
        case CmpOp::EQ:
            Instgen::ite("eq");
            Instgen::moveq(target_Reg, 1);
            Instgen::movne(target_Reg, 0);
            break;
        case CmpOp::NE:
            Instgen::ite("ne");
            Instgen::movne(target_Reg, 1);
            Instgen::moveq(target_Reg, 0);
            break;
    }
}

void Codegen::gen_zext(ZextInst *inst) {
    auto target_Reg = get(inst, true);
    auto cond_inst = inst->get_operand(0).get();
    bool rhs_unsaved = not reg_mapping.count(cond_inst);

    auto is_icmp = dynamic_cast<CmpInst *>(cond_inst) != nullptr;
    auto is_fcmp = dynamic_cast<FCmpInst *>(cond_inst) != nullptr;
    if (is_icmp and rhs_unsaved) {
        auto cmp_inst = dynamic_cast<CmpInst *>(cond_inst);

        // 生成前一句 cmp
        // pos -= 2;
        // gen_cmp(cmp_inst);
        // pos += 2;

        gen_cmp_move(cmp_inst, target_Reg);
    } else if (is_fcmp and rhs_unsaved) {
        auto fcmp_inst = dynamic_cast<FCmpInst *>(cond_inst);

        // pos -= 2;
        // gen_fcmp(fcmp_inst);
        // pos += 2;

        gen_fcmp_move(fcmp_inst, target_Reg);
    } else {
        move(target_Reg, cond_inst);
    }
}

void Codegen::gen_fptosi(FpToSiInst *inst) {
    auto rhs = inst->get_operand(0).get();
    // generate_function_call("__fixsfsi", {rhs}, inst);

    Reg rhs_float_reg = temp_float_lhs_reg;
    if (not dynamic_cast<ConstantFP *>(rhs))
        rhs_float_reg = get(rhs);

    Reg temp_reg = temp_float_rhs_reg;
    Reg dest_reg = get(inst, true);

    move(rhs_float_reg, rhs);
    Instgen::ftintrz_w_s(temp_reg, rhs_float_reg);
    Instgen::reg_mov(dest_reg, temp_reg);
}

void Codegen::gen_sitofp(SiToFpInst *inst) {
    auto rhs = inst->get_operand(0).get();
    bool rhs_unsaved = not reg_mapping.count(rhs);
    auto is_icmp = dynamic_cast<CmpInst *>(rhs);
    auto is_fcmp = dynamic_cast<FCmpInst *>(rhs);
    Reg dest_float_reg = get(inst, true);

    // 当 rhs 是 cmp 或 fcmp 时，生成前一句，并将 i1 转换成 float
    if (is_icmp and rhs_unsaved) {
        auto cmp_inst = dynamic_cast<CmpInst *>(rhs);

        // pos -= 2;
        // gen_cmp(cmp_inst);
        // pos += 2;

        gen_cmp_move(cmp_inst, temp_lhs_reg);
        Instgen::reg_mov(dest_float_reg, temp_lhs_reg);
        Instgen::ffint_s_w(dest_float_reg, dest_float_reg);
    } else if (is_fcmp and rhs_unsaved) {
        auto fcmp_inst = dynamic_cast<FCmpInst *>(rhs);

        // pos -= 2;
        // gen_fcmp(fcmp_inst);
        // pos += 2;

        gen_fcmp_move(fcmp_inst, temp_lhs_reg);
        Instgen::reg_mov(dest_float_reg, temp_lhs_reg);
        Instgen::ffint_s_w(dest_float_reg, dest_float_reg);
    } else {
        move(dest_float_reg, rhs);
        Instgen::ffint_s_w(dest_float_reg, dest_float_reg);
    }
}

void Codegen::gen_function_call(std::string func_name,
                                std::vector<Value *> all_value,
                                Instruction *dest_inst,
                                Instruction *push_pop_key) {
    // std::for_each(all_value.begin(), all_value.end(), [this](Value *v) {
    //     if (reg_mapping.count(v))
    //         get(v);
    // });
    push_pop_key = dest_inst ? dest_inst : push_pop_key;
    std::vector<Reg> regs, float_regs;
    for (auto reg : call_saves[push_pop_key]) {
        if (not reg.is_float)
            regs.push_back(reg);
        else
            float_regs.push_back(reg);
    }
    // sort float_regs
    std::sort(float_regs.begin(), float_regs.end(), [](Reg a, Reg b) { return a.id < b.id; });

    int regs_size = regs.size() * 8;
    int total_save_size = regs.size() * 8 + float_regs.size() * 8;

    for (int i = 0; i < regs.size(); i++)
        Instgen::st_d(regs[i], Reg::ID::sp, -total_save_size + i * 8);
    for (int i = 0; i < float_regs.size(); i++)
        Instgen::fst_s(float_regs[i], Reg::ID::sp, -total_save_size + regs_size + i * 8);
    Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, -total_save_size);

    // 不能写成 Instgen::vpush(float_regs)
    // vpush 和 vpop 必须连续，且最好对齐 8 字节
    // TODO: 可能改成 vstr 和 vldr
    stack_growth_since_entry += total_save_size;

    std::vector<std::pair<armval, armval>> moves_int, moves_float;
    int save_index_int = 0, save_index_float = 0;
    std::vector<Value *> save_on_stack;
    for (size_t i = 0; i < all_value.size(); ++i) {
        auto value = all_value[i];
        auto is_float = value->get_type()->is_float_type();
        auto &moves = is_float ? moves_float : moves_int;
        int max_save_index = is_float ? RegAlloc::fp_arg_max_save_index : RegAlloc::int_arg_max_save_index;
        int &save_index = is_float ? save_index_float : save_index_int;

        if (save_index > max_save_index) {
            save_on_stack.push_back(value);
        } else {
            // move(Reg(i), all_value[i]); // like when resolving phis, we might have cyclic references
            auto target_reg = Reg(save_index, is_float);
            auto cur = get_loc(value);
            if (armval{target_reg} != cur)
                moves.push_back({target_reg, cur});
            save_index++;
        }
    }
    int arg_stack_size = save_on_stack.size();
    if (arg_stack_size % 2 == 1)
        arg_stack_size++;
    // new offset added to sp!
    Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, -arg_stack_size * 8);
    stack_growth_since_entry += arg_stack_size * 8;

    // if (arg_stack_size > 0)
    //     LOG_WARNING << "arg_stack_size = " << arg_stack_size;

    for (int i = 0; i < save_on_stack.size(); ++i) {
        auto value = save_on_stack[i];
        auto is_float = value->get_type()->is_float_type();
        if (not is_float) {
            Reg temp_reg = temp_lhs_reg;
            move(temp_reg, value);
            Instgen::st_d(temp_reg, Reg::ID::sp, i * 8);
        } else {
            Reg temp_reg = temp_float_lhs_reg;
            move(temp_reg, value);
            Instgen::fst_s(temp_reg, Reg::ID::sp, i * 8);
        }
    }
    // print moves_int
    for (auto &move : moves_int)
        LOG_DEBUG << "move " << move.first << " to " << move.second;

    parallel_mov(moves_int);
    parallel_mov(moves_float);
    Instgen::bl(func_name);

    if (dest_inst != nullptr) {
        // get(dest_inst) 要在 move(Reg(i), all_value[i]) 之后执行
        auto dest_reg = get(dest_inst, true);
        if (not dest_inst->get_type()->is_float_type())
            Instgen::mov(dest_reg, Reg::ID::ret);
        else
            Instgen::reg_mov(dest_reg, Reg(0, true));
    }

    Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, arg_stack_size * 8);
    stack_growth_since_entry -= arg_stack_size * 8;

    // if (arg_stack_size > 0)
    //     LOG_WARNING << "arg_stack_size = " << arg_stack_size;

    stack_growth_since_entry -= total_save_size;

    Instgen::addi_d(Reg::ID::sp, Reg::ID::sp, total_save_size);

    for (int i = 0; i < regs.size(); i++)
        Instgen::ld_d(regs[i], Reg::ID::sp, -total_save_size + i * 8);
    for (int i = 0; i < float_regs.size(); i++)
        Instgen::fld_s(float_regs[i], Reg::ID::sp, -total_save_size + regs_size + i * 8);
}

void Codegen::gen_call(CallInst *inst) {
    auto func = inst->get_callee();
    auto func_name = func->get_name();

    std::vector<Value *> all_value;
    for (int i = 0; i < func->get_num_of_args(); ++i)
        all_value.push_back(inst->get_operand(i + 1).get());

    Instruction *dest_inst = nullptr;
    if (not inst->get_type()->is_void_type()) {
        if (reg_mapping.count(inst))
            dest_inst = inst;
        else {
            if (not inst->get_use_list().empty()) {
                LOG_ERROR << inst->print() << "of " << inst->get_function()->get_name()
                          << " used and stored in memory\n";
                exit(211);
              
            }
        }
    }
    gen_function_call(func_name, all_value, dest_inst, inst);
}

void Codegen::gen_ret(ReturnInst *inst) {
    if (inst->is_void_ret())
        return;
    auto value = inst->get_operand(0).get();
    auto is_float = value->get_type()->is_float_type();
    // if (not is_float)
    move(Reg::ID::ret, value);
    // else
    //     move(Reg(0, true), value);

    // addi.w	$r12,$r0,111
    // or	$r4,$r12,$r0
}

void Codegen::gen_br(BasicBlock *bb, BranchInst *inst) {
    //直接跳转
    if (inst->get_num_operand() == 1) {
        auto target = dynamic_cast<BasicBlock *>(inst->get_operand(0).get());
        resolve_split(pos + 1);
        resolve_moves(bb, target);
        Instgen::b(get_label_name(target));
    } 
    //带判断条件的跳转
    else {
        auto target_then = dynamic_cast<BasicBlock *>(inst->get_operand(1).get());
        auto target_else = dynamic_cast<BasicBlock *>(inst->get_operand(2).get());
        auto label_then = get_label_name(target_then);
        auto label_else = get_label_name(target_else);

        auto cond_inst = inst->get_operand(0).get();
        //使用到了LIR吗 我感觉IR不全是MIR
        bool update_flag = m->level == Module::LIR and isa<BinaryInst>(cond_inst);
        //常数 或cmp比较后的0|1 或已经保存在寄存器的指令
        auto is_int_cond = update_flag or dynamic_cast<ConstantInt *>(cond_inst) or
                           dynamic_cast<CmpInst *>(cond_inst) or
                           (reg_mapping.count(cond_inst) and get(cond_inst).valid());
        if (is_int_cond) {
            auto cmp_inst = dynamic_cast<CmpInst *>(cond_inst);
            auto constant_bool = dynamic_cast<ConstantInt *>(cond_inst);
            Reg rhs_reg = temp_lhs_reg;
            CmpOp cond = CmpOp::NE;  // move compare result from elsewhere
            //因为是cmp的 所以当时会将cmp的结果放在某一寄存器
            if (reg_mapping.count(cond_inst) and not update_flag) {
                rhs_reg = get(cond_inst);
                // move(temp_rhs_reg,reg,cond);
                Instgen::mov(temp_rhs_reg,rhs_reg);
                //修改arm的cpsr寄存器 我们使用loong可以不用修改 loong没有cpsr寄存器 使用temp_rhs_reg
                // Instgen::tst(cond_reg, 1);
            } else if (constant_bool) {
                Instgen::mov(temp_rhs_reg, constant_bool->get_value());
                // Instgen::tst(temp_lhs_reg, 1);
                // rhs_reg = temp_lhs_reg;
            } else if (update_flag)
                cond = inst->get_cmp_op();
            else
                cond = cmp_inst->get_cmp_op();

            auto inner_label = get_label_name(inst->get_parent()) + "_inner";

            // <    slt l,r     1 represents l < r
            // >    slt r,l     1 represents l > r
            // <=   slt r,l     0 represents l <= r
            // >=   slt l,r     0 represents l >= r
            // ==   sub l,r     0 represents l == r
            // !=   sub l,r     1 represents l != r
            LOG_DEBUG<<"may yes";
            switch (cond) {
                case CmpOp::LT:
                case CmpOp::GT:
                case CmpOp::NE:
                    Instgen::beqz(temp_rhs_reg, inner_label);
                    break;
                case CmpOp::LE:
                case CmpOp::GE:
                case CmpOp::EQ:
                    Instgen::bnez(temp_rhs_reg, inner_label);
                    break;
            }
            LOG_DEBUG<<"maybe not";

            resolve_split(pos + 1);
            resolve_moves(bb, target_then, cond._to_string());

            Instgen::b(label_then);
            output << "\t" << inner_label << ":\n";
        } else {
            auto fcmp_inst = dynamic_cast<FCmpInst *>(cond_inst);

            // pos -= 2;
            // gen_fcmp(fcmp_inst); // like gen_cmp, can't call it in here
            // pos += 2;

            resolve_split(pos + 1);
            resolve_moves(bb, target_then, fcmp_inst->get_cmp_op()._to_string());  // ? 位置不同

            auto inner_label = get_label_name(inst->get_parent()) + "_inner";
            // bceqz "$fcc0"
            // bcnez
            switch (fcmp_inst->get_cmp_op()) {
                case CmpOp::LT:
                case CmpOp::GT:
                case CmpOp::NE:
                    Instgen::bceqz("$fcc0", inner_label);
                    break;
                case CmpOp::LE:
                case CmpOp::GE:
                case CmpOp::EQ:
                    Instgen::bcnez("$fcc0", inner_label);
                    break;
            }

            Instgen::b(label_then);
            output << "\t" << inner_label << ":\n";
        }
        // phi
        resolve_moves(bb, target_else);

        Instgen::b(label_else);
    }
}

void Codegen::move(Reg r, int val, std::string_view cond) {
    Instgen::mov(r, val, cond);
}

void Codegen::move(Reg r, Value *val, std::string_view cond) {
    assert(r.valid());
    assert(val);
    if (auto const_val = dynamic_cast<Constant *>(val)) {
        if (const_val->is_constant_int()) {
            auto const_int = dynamic_cast<ConstantInt *>(val)->get_value();
            if (not r.is_float) {
                move(r, const_int, cond);
            } else {
                //浮点寄存器
                auto temp_reg = temp_lhs_reg;
                move(temp_reg, const_int, cond);
                Instgen::reg_mov(r, temp_reg, cond);
            }
        } else {
            auto const_fp = dynamic_cast<ConstantFP *>(val);
            // if (not r.is_float)
            //     Instgen::ldr(r, get_label_name_float(const_fp));
            // else
            //     Instgen::fld_s(r, get_label_name_float(const_fp));

            auto float_value = const_fp->get_value();
            auto int_value = *(int *)&float_value;

            // WARN: 使用 temp_rhs_reg 可能引起问题
            Instgen::mov(temp_rhs_reg, int_value, cond);
            Instgen::reg_mov(r, temp_rhs_reg, cond);
        }
    } else if (auto gl = dynamic_cast<GlobalVariable *>(val)) {
        LOG_DEBUG << "Global variable " << gl->get_name();
        Instgen::la_local(r, Instgen::label{gl->get_name(), 0});
        // Instgen::la_local(r, Instgen::label{gl->get_name(), GOT.at(gl) * 4});
    } else {
        if (reg_mapping.count(val) and get(val).valid()) {
            // bool is_float = val->get_type()->is_float_type();
            // if (not r.is_float and not is_float)
            Instgen::reg_mov(r, get(val), cond);
        } else {
            auto inst = dynamic_cast<Instruction *>(val);
            auto arg = dynamic_cast<Argument *>(val);
            assert((inst or arg) and "must be an instruction or an argument");
            if (inst and inst->is_alloca()) {
                //感觉像相对地址+起始地址
                auto size = sp_offset.at(inst) + stack_growth_since_entry;
                if (encode_arm_immediate(size))
                    Instgen::addi_d(r, Reg::ID::sp, size);
                else {
                    move(r, size, cond);
                    Instgen::add_d(r, r, Reg::ID::sp);
                }
            } else {
                if (not r.is_float)
                    Instgen::ld_d(r, Reg::ID::sp, sp_offset.at(val) + stack_growth_since_entry, cond);
                else
                    Instgen::fld_s(r, Reg::ID::sp, sp_offset.at(val) + stack_growth_since_entry, cond);
                LOG_DEBUG << '"' << val->print() << '"' << " is in memory";
            }
        }
    }
}

/// \brief value `to` is assumed to be output (get at pos+1)
void Codegen::move(Value *to, Value *from, std::string_view cond) {
    Reg r = get(to, true);
    move(r, from, cond);
}

void Codegen::resolve_moves(BasicBlock *frombb, BasicBlock *tobb) { resolve_moves(frombb, tobb, ""); }

void Codegen::resolve_moves(BasicBlock *frombb, BasicBlock *tobb, std::string_view cond) {
    parallel_mov(move_mapping[{frombb, tobb}], cond);
    move_mapping.erase({frombb, tobb});
}

void Codegen::parallel_mov(std::vector<std::pair<armval, armval>> &moves_input, std::string_view cond) {
    Reg to;
    armval to1, from;
    std::multimap<int, int> edges;  // graph maps FROM to TO!!
    std::set<int, std::greater<int>> nodes;
    std::set<std::pair<armval, armval>> set_int, set_float;
    std::vector<std::pair<armval, armval>> moves_float, moves_int;
    for (auto &[to, from] : moves_input) {
        if (to.index() == int_variant_index or (std::holds_alternative<Reg>(to) and !std::get<Reg>(to).is_float)) {
            moves_int.push_back({to, from});
            set_int.insert({to, from});
        } else {
            moves_float.push_back({to, from});
            set_float.insert({to, from});
        }
    }
    assert(moves_int.size() == set_int.size() and "moves_int is not unique");
    assert(moves_float.size() == set_float.size() and "moves_float is not unique");
    // 分三次 mov
    // 1. all -> memory, in the meantime find moves from reg to reg
    // 2. do a topological sort, move in dataflow order
    // 3. move value*/memory stuff to register
    for (int i : {0, 1}) {
        nodes.clear();
        edges.clear();
        bool is_int = i == 0;
        auto &moves = is_int ? moves_int : moves_float;

        std::function<void(Reg, Reg, size_t, std::string_view)> str =
            is_int ? [](Reg a, Reg b, size_t c, std::string_view d) -> void { Instgen::st_d(a, b, c, d); }
                   : [](Reg a, Reg b, size_t c, std::string_view d) -> void { Instgen::fst_s(a, b, c, d); };
        std::function<void(Reg, Reg, size_t, std::string_view)> ldr =
            is_int ? [](Reg a, Reg b, size_t c, std::string_view d) -> void { Instgen::ld_d(a, b, c, d); }
                   : [](Reg a, Reg b, size_t c, std::string_view d) -> void { Instgen::fld_s(a, b, c, d); };
        for (auto &b : moves) {
            std::tie(to1, from) = b;
            if (std::holds_alternative<Reg>(to1)) {
                to = std::get<Reg>(to1);
                std::visit(overloaded{[&](Reg from) {
                                          nodes.insert({from.id, to.id});
                                          edges.insert({from.id, to.id});
                                          LOG_DEBUG << "move " << from << " to " << to
                                                    << " in visit  from.id = " << from.id << " to.id = " << to.id;
                                      },
                                      [](Value *_) { return; },
                                      [](size_t _) { return; }},
                           from);
            } else {
                size_t offset = is_int ? std::get<int_variant_index>(to1) : std::get<float_variant_index>(to1);
                // don't need to use vldr/vstr if float?
                std::visit(
                    overloaded{[&](Reg from) { str(from, Reg::ID::sp, offset + stack_growth_since_entry, cond); },
                               [&, this](Value *c) {
                                   Reg temp = is_int ? temp_lhs_reg : temp_float_lhs_reg;
                                   move(temp, c, cond);
                                   str(temp, Reg::ID::sp, offset + stack_growth_since_entry, cond);
                               },
                               [&, cond](size_t from) {
                                   // FIXME: 在一些很刁钻的场合下，不可以直接load/store，也需要排序
                                   Reg temp = is_int ? temp_lhs_reg : temp_float_lhs_reg;
                                   ldr(temp, Reg::ID::sp, from + stack_growth_since_entry, cond);
                                   str(temp, Reg::ID::sp, offset + stack_growth_since_entry, cond);
                               }},
                    from);
            }
        }

        std::string nodes_str;
        for (auto &n : nodes)
            nodes_str += std::to_string(n) + " ";
        LOG_DEBUG << "nodes: " << nodes_str;

        for (auto &[from, to] : edges)
            LOG_DEBUG << "edge: " << from << " -> " << to;

        std::vector<std::pair<int, int>> rev_topo = topo_sort(nodes, edges, is_int ? move_t::ireg : move_t::freg);

        LOG_DEBUG << "topo sort result:";
        for (auto &[from, to] : rev_topo) {
            LOG_DEBUG << "move " << Reg(from) << " to " << Reg(to);
        }

        if (i == 0)
            for (auto [from, to] : rev_topo)
                Instgen::mov(Reg(to), Reg(from), cond);
        else
            for (auto [from, to] : rev_topo)
                Instgen::reg_mov(Reg(to, true), Reg(from, true), cond);

        for (auto &b : moves) {
            std::tie(to1, from) = b;
            if (!std::holds_alternative<Reg>(to1))
                continue;
            to = std::get<Reg>(to1);
            std::visit(overloaded{[](Reg _) { return; },
                                  [&](Value *val) { move(to, val, cond); },
                                  [&, cond, to, this](size_t offset) {
                                      ldr(to, Reg::ID::sp, offset + stack_growth_since_entry, cond);
                                  }},
                       from);
        }
    }
}
// 要判断的有点多
// ptr是不是alloca
// 有没有offset位，offset是常数还是变量（变量和sp的话得加条指令）
void Codegen::store(Value *val, Value *ptr, StoreInst *store) {
    auto is_float = val->get_type()->is_float_type();
    if (not is_float) {
        Reg r = reg_mapping.count(val) and get(val).valid() ? get(val) : temp_lhs_reg;

        move(r, val);
        if (reg_mapping.count(ptr)) {
            Reg p = get(ptr);
            move(p, ptr);

            if (store->get_num_operand() > 2) {
                auto op2 = store->get_operand(2).get();
                if (reg_mapping.count(op2)) {
                    LOG_DEBUG<<"reg_mapping.count(op2)";
                    Reg offset = get(op2);
                    offset.shift_n = store->get_op2_shift_bits();
                    offset.op = store->get_op2_shift_type();
                    Instgen::stx_w(r,p,offset);
                    // Instgen::str(r, p, offset);
                } else {
                    LOG_DEBUG<<"reg_mapping not count";
                    // constant offset, add with sp if it's alloca
                    ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                    // Instgen::str(r, p, offset->get_value());
                    Instgen::st_w(r,p,offset->get_value());
                }
            } else {
                Instgen::st_w(r, p);
            }
        } else {
            LOG_DEBUG<<"ptr not in regmapping ";
            assert(isa<AllocaInst>(ptr));
            if (store->get_num_operand() > 2) {
                auto op2 = store->get_operand(2).get();
                if (reg_mapping.count(op2)) {
                    Reg offset = get(op2);
                    offset.shift_n = store->get_op2_shift_bits();
                    offset.op = store->get_op2_shift_type();
                    // Instgen::add(temp_rhs_reg, Reg::ID::sp, offset);
                    Instgen::add_d(temp_rhs_reg,Reg::ID::sp,offset);
                    Instgen::st_w(r,temp_rhs_reg,stack_growth_since_entry+sp_offset.at(ptr));
                    // Instgen::str(r, temp_rhs_reg, stack_growth_since_entry + sp_offset.at(ptr));
                } else {
                    ConstantInt *offset = dynamic_cast<ConstantInt *>(store->get_operand(2).get());
                    Instgen::st_w(r,Reg::ID::sp,stack_growth_since_entry+sp_offset.at(ptr)+offset->get_value());
                    // Instgen::str(r, Reg::ID::sp, stack_growth_since_entry + sp_offset.at(ptr) + offset->get_value());
                }
            } else {
                Instgen::st_w(r,Reg::ID::sp,sp_offset.at(ptr) + stack_growth_since_entry);
                // Instgen::str(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry);
            }
        }
    } else {
        //2023 TODO HJ
        Reg r = reg_mapping.count(val) and get(val).valid() ? get(val) : temp_float_lhs_reg;
        move(r, val);
        if (reg_mapping.count(ptr)) {
            Reg p = get(ptr);
            move(p, ptr);
            if (store->get_num_operand() > 2) {
                auto op2 = store->get_operand(2).get();
                assert(isa<Constant>(op2));
                // constant offset, add with sp if it's alloca
                ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                Instgen::fst_s(r, p, offset->get_value());
            } else {
                Instgen::fst_s(r, p);
            }
        } else {
            assert(isa<AllocaInst>(ptr));
            if (store->get_num_operand() > 2) {
                auto op2 = store->get_operand(2).get();
                ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                Instgen::fst_s(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry + offset->get_value());
            } else {
                Instgen::fst_s(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry);
            }
        }
    }
}

void Codegen::load(Value *val, Value *ptr, LoadInst *load) {
    auto is_float = val->get_type()->is_float_type();
    Reg r = get(val, true);

    if (not is_float) {
        // int, ptr is not alloca
        if (reg_mapping.count(ptr)) {
            Reg p = get(ptr);
            move(p, ptr);
            // with an offset
            if (load->get_num_operand() > 1) {
                auto op2 = load->get_operand(1).get();
                if (reg_mapping.count(op2)) {
                    Reg offset = get(op2);
                    offset.shift_n = load->get_op2_shift_bits();
                    offset.op = load->get_op2_shift_type();
                    // Instgen::ldr(r, p, offset);
                    Instgen::ldx_w(r,p,offset);
                } else {
                    // constant offset, add with sp if it's alloca
                    ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                    Instgen::ld_w(r,p,offset->get_value());
                    // Instgen::ldr(r, p, offset->get_value());
                }
            } else {
                Instgen::ld_w(r, p);
            }
        } else {  // alloca
            assert(isa<AllocaInst>(ptr));
            // alloca + offset
            LOG_DEBUG<<load->get_num_operand();
            if (load->get_num_operand() > 1) {
                auto op2 = load->get_operand(1).get();
                if (reg_mapping.count(op2)) {
                    Reg offset = get(op2);
                    offset.shift_n = load->get_op2_shift_bits();
                    offset.op = load->get_op2_shift_type();
                    // Instgen::add(temp_rhs_reg, Reg::ID::sp, offset);
                    Instgen::add_d(temp_rhs_reg,Reg::ID::sp,offset);
                    LOG_DEBUG<<stack_growth_since_entry<<" "<<sp_offset.at(ptr);
                    Instgen::ld_w(r, temp_rhs_reg, stack_growth_since_entry + sp_offset.at(ptr));
                    // Instgen::ldr(r, temp_rhs_reg, stack_growth_since_entry + sp_offset.at(ptr));
                } else {
                    ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                    LOG_DEBUG<<stack_growth_since_entry<<" "<<sp_offset.at(ptr)<<" "<<offset->get_value();
                    Instgen::ld_w(r, Reg::ID::sp, stack_growth_since_entry + sp_offset.at(ptr) + offset->get_value());
                    // Instgen::ldr(r, Reg::ID::sp, stack_growth_since_entry + sp_offset.at(ptr) + offset->get_value());
                }
            } else {
                Instgen::ld_w(r,Reg::ID::sp,sp_offset.at(ptr)+stack_growth_since_entry);
                // Instgen::ldr(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry);
            }
        }
    } else {
        // TODO 2023 HJ
        // float, not sp
        if (reg_mapping.count(ptr)) {
            Reg p = get(ptr);
            move(p, ptr);
            // offset, consts only
            if (load->get_num_operand() > 1) {
                auto op2 = load->get_operand(1).get();
                assert(isa<Constant>(op2));
                // constant offset, add with sp if it's alloca
                ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
                Instgen::fld_s(r, p, offset->get_value());
            } else {
                Instgen::fld_s(r, p);
            }
        } else {
            assert(isa<AllocaInst>(ptr));
            if (load->get_num_operand() > 1) {
                auto op2 = load->get_operand(1).get();
                ConstantInt *offset = dynamic_cast<ConstantInt *>(op2);
            
                Instgen::fld_s(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry + offset->get_value());
            } else {
                Instgen::fld_s(r, Reg::ID::sp, sp_offset.at(ptr) + stack_growth_since_entry);
            }
        }
    }
}

/// \param edges 多个phi语句可能依赖于同一个输入；在同一个bb中不同的phi不会有相同的寄存器
std::vector<std::pair<int, int>> topo_sort(std::set<int, std::greater<int>> &nodes,
                                           std::multimap<int, int> &edges,
                                           move_t temp_reg_type) {
    for (auto [from, to] : edges) {
        LOG_DEBUG << from << " -> " << to;
        // std::cout << "entry: " << from << " -> " << to << std::endl;
    }

    std::vector<std::pair<int, int>> ret{};
    std::set<std::pair<int, int>> set_ret;
    std::vector<int> sorted;
    std::map<int, int> visited;
    int ret_n{-1};
    int temp_reg_no{Codegen::temp_rhs_reg.id};
    std::vector<std::pair<int, int>> nret{};
    std::set<int> temp_pool;
    switch (temp_reg_type) {
        case move_t::ireg:
            temp_pool.insert({22, 23, 24, 25, 26, 27});
            break;
        case move_t::freg:
            temp_pool.insert({30, 31});
            break;
    }
    // do {
    visited.clear();
    sorted.clear();
    while (not nodes.empty()) {
        int n = *nodes.begin();
        // nodes.erase(nodes.begin());
        if (visited[n])
            continue;
        // std::cout << n << std::endl;
        // y combinator, recursive lambda
        ret_n = boost::hana::fix([&](auto dfs, int n) -> int {
            if (visited[n] == 2)
                return -1;
            if (visited[n] == 1) {
                LOG_INFO << "non DAG, " << n << " in cycle, adding temp register";
                // nret.push_back({n, })
                visited[n] = 0;
                return n;
            }
            visited[n] = 1;
            auto [begin, end] = edges.equal_range(n);
            for (auto i = begin; i != end; ++i)
                if (auto r = dfs(i->second); r != -1) {
                    visited[n] = 0;  // reset
                    return r;
                }
            visited[n] = 2;
            sorted.push_back(n);
            if ((temp_reg_type == move_t::ireg and n >= 10) or (temp_reg_type == move_t::freg and n >= 30))
                temp_pool.insert(n);
            for (auto i = begin; i != end; ++i) {
                nret.push_back(*i);
                set_ret.insert(*i);
            }
            nodes.erase(n);
            // edges.erase(n); // 可不敢乱删
            return -1;
        })(n);
        if (ret_n != -1) {
            // for (auto nn : nodes)
            //     std::cout << nn << ", ";
            // std::cout << "\n";
            nodes.insert(temp_reg_no = *temp_pool.begin());  // 循环的时候insert??? 你不报错谁报错
            visited[temp_reg_no] = 0;
            edges.erase(temp_reg_no);
            temp_pool.erase(temp_pool.begin());
            const auto [begin, end] = edges.equal_range(ret_n);
            std::vector<std::pair<int, int>> ret_n_edges;
            for (auto i = begin; i != end; i++) {
                ret_n_edges.push_back({temp_reg_no, i->second});
            }
            edges.erase(ret_n);
            for (auto t : ret_n_edges)
                edges.insert(t);
            nret.push_back({ret_n, temp_reg_no});
            set_ret.insert({ret_n, temp_reg_no});
        }
    }
    std::ostringstream os;
    os << "node size: " << nodes.size() << ", edges: " << edges.size() << ".\ttopo sort result: ";
    std::ostream_iterator<int> osi(os, ", ");
    std::copy(sorted.begin(), sorted.end(), osi);
    LOG_DEBUG << os.str();
    assert(nret.size() == set_ret.size());
    for (auto [from, to] : nret)
        LOG_DEBUG << from << " -> " << to;
    return nret;
}

#include "codegen.hh"

#include <assert.h>

#undef NDEBUG

std::ofstream *Instgen::output{nullptr};
template <typename... Args>
void Instgen::push(Args... args) {
    (*output) << "\tpush\t"
              << "{" << (... << args) << "}\n";
}

// void Instgen::push(std::vector<Reg> args) {
//     if (args.empty())
//         return;
//     (*output) << "\tpush\t{";
//     (*output) << args.front();
//     for (auto it = std::next(args.begin()); it != args.end(); ++it) {
//         (*output) << ", " << *it;
//     }
//     (*output) << "}\n";
// }

void Instgen::push(std::vector<Reg> args) {
    for (auto arg : args)
        if (!(arg.valid() and not arg.is_float))
            exit(26);
    if (args.empty())
        return;
    (*output) << "\tpush\t{";
    (*output) << *args.begin();
    for (auto it = std::next(args.begin()); it != args.end(); ++it) {
        (*output) << ", " << *it;
    }
    (*output) << "}\n";
}

void Instgen::push_caller_saved() { (*output) << "\tpush\t{r0-r3}\n"; }

void Instgen::pop_caller_saved() { (*output) << "\tpop\t{r0-r3}\n"; }

template <typename... Args>
void Instgen::pop(Args... args) {
    (*output) << "\tpop\t"
              << "{" << (... << args) << "}\n";
}

// void Instgen::pop(std::vector<Reg> args) {
//     if (args.empty())
//         return;
//     (*output) << "\tpop\t{";
//     (*output) << args.front();
//     for (auto it = std::next(args.begin()); it != args.end(); ++it) {
//         (*output) << ", " << *it;
//     }
//     (*output) << "}\n";
// }

void Instgen::pop(std::vector<Reg> args) {
    for (auto arg : args)
        if (!(arg.valid() and not arg.is_float))
            exit(27);
    if (args.empty())
        return;
    (*output) << "\tpop\t{";
    (*output) << *args.begin();
    for (auto it = std::next(args.begin()); it != args.end(); ++it) {
        (*output) << ", " << *it;
    }
    (*output) << "}\n";
}

void Instgen::bl(std::string_view sv) { (*output) << "\tbl\t" << sv << "\n"; }

// template <typename... Args>
// void assert_regs(Args... args) {}

void Instgen::mov(Reg r1, Reg r2, std::string_view cond) {
    if (!(r1.valid() and not r1.is_float))
        exit(28);
    if (!(r2.valid() and not r2.is_float))
        exit(29);
    if (r1 != r2)
        (*output) << "\tmov" << cond << "\t" << r1 << ", " << r2 << "\n";
}

void Instgen::mov(Reg r, int i, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(30);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(31);
    (*output) << "\tmov" << cond << "\t" << r << ", #" << i << "\n";
}

void Instgen::movt(Reg r, int i, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(32);
    (*output) << "\tmovt" << cond << "\t" << r << ", #" << i << "\n";
}

void Instgen::movw(Reg r, int i, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(33);
    (*output) << "\tmovw" << cond << "\t" << r << ", #" << i << "\n";
}

void Instgen::sub(Reg r1, Reg r2, int n, std::string_view cond) {
    if (!(r1.valid() and not r1.is_float))
        exit(34);
    if (!(r2.valid() and not r2.is_float))
        exit(35);
    if (r1 == r2 and n == 0)
        return;
    (*output) << "\tsub" << cond << "\t" << r1 << ", " << r2 << ", #" << n << "\n";
}

void Instgen::add(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::add(Reg dest, Reg r1, Reg r2, std::string_view cond, std::string_view suffix) {
    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", " << r2 << ", " << suffix << "\n";
}

void Instgen::sub(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(39);
    if (!(r1.valid() and not r1.is_float))
        exit(40);
    if (!(r2.valid() and not r2.is_float))
        exit(41);
    (*output) << "\tsub" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}
void Instgen::rsub(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(39);
    if (!(r1.valid() and not r1.is_float))
        exit(40);
    if (!(r2.valid() and not r2.is_float))
        exit(41);
    (*output) << "\trsb" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::mul(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(42);
    if (!(r1.valid() and not r1.is_float))
        exit(43);
    if (!(r2.valid() and not r2.is_float))
        exit(44);
    (*output) << "\tmul" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::mla(Reg dest, Reg r1, Reg r2, Reg r3) {
    (*output) << "\tmla\t" << dest << ", " << r1 << ", " << r2 << ", " << r3 << "\n";
}

void Instgen::sdiv(Reg dest, Reg r1, Reg r2) {
    assert(dest.valid() and not dest.is_float);
    (*output) << "\tsdiv\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::smmul(Reg dest, Reg r1, Reg r2) {
    assert(dest.valid() and not dest.is_float);
    (*output) << "\tsmmul\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::mls(Reg dest, Reg r1, Reg r2, Reg r3) {
    assert(dest.valid() and not dest.is_float);
    (*output) << "\tmls\t" << dest << ", " << r1 << ", " << r2 << ", " << r3 << "\n";
}

void Instgen::cmp(Reg r1, Reg r2) {
    if (!(r1.valid() and not r1.is_float))
        exit(45);
    if (!(r1.valid() and not r2.is_float))
        exit(46);
    (*output) << "\tcmp\t" << r1 << ", " << r2 << "\n";
}

void Instgen::cmp(Reg r, int n) { (*output) << "\tcmp\t" << r << ", #" << n << "\n"; }
void Instgen::tst(Reg r1, int n) { (*output) << "\ttst\t" << r1 << ", #" << n << "\n"; }

void Instgen::add(Reg dest, Reg r1, int imm, std::string_view cond) {
    if (!(r1.valid() and not dest.is_float))
        exit(47);
    if (!(not r1.is_float))
        exit(48);
    if (dest == r1 and imm == 0)
        return;
    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", #" << imm << "\n";
}
void Instgen::lsl(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(49);
    if (!(r1.valid() and not r1.is_float))
        exit(50);
    if (!(r2.valid() and not r2.is_float))
        exit(51);
    (*output) << "\tlsl" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}
void Instgen::lsr(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(52);
    if (!(r1.valid() and not r1.is_float))
        exit(53);
    if (!(r2.valid() and not r2.is_float))
        exit(54);
    (*output) << "\tlsr" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::asr(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    if (!(dest.valid() and not dest.is_float))
        exit(55);
    if (!(r1.valid() and not r1.is_float))
        exit(56);
    if (!(r2.valid() and not r2.is_float))
        exit(57);
    (*output) << "\tasr" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::and_(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\tand" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::and_(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tand\t" << dest << ", " << r1 << ", #" << imm << "\n";
}

void Instgen::bic(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tbic\t" << dest << ", " << r1 << ", #" << imm << "\n";
}

void Instgen::lsl(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tlsl\t" << dest << ", " << r1 << ", #" << imm << "\n";
}

void Instgen::lsr(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tlsr\t" << dest << ", " << r1 << ", #" << imm << "\n";
}

void Instgen::asr(Reg dest, Reg r1, int n) { (*output) << "\tasr\t" << dest << ", " << r1 << ", #" << n << "\n"; }

void Instgen::ite(std::string_view sv) { (*output) << "\tite\t" << sv << "\n"; }
void Instgen::bx(Reg r) { (*output) << "\tbx\t" << r << "\n"; }
void Instgen::b(std::string_view sv) { (*output) << "\tb\t" << sv << "\n"; }
void Instgen::beq(std::string_view sv) { (*output) << "\tbeq\t" << sv << "\n"; }
void Instgen::bne(std::string_view sv) { (*output) << "\tbne\t" << sv << "\n"; }
void Instgen::bgt(std::string_view sv) { (*output) << "\tbgt\t" << sv << "\n"; }
void Instgen::bge(std::string_view sv) { (*output) << "\tbge\t" << sv << "\n"; }
void Instgen::blt(std::string_view sv) { (*output) << "\tblt\t" << sv << "\n"; }
void Instgen::ble(std::string_view sv) { (*output) << "\tble\t" << sv << "\n"; }

void Instgen::bhi(std::string_view sv) { (*output) << "\tbhi\t" << sv << "\n"; }
void Instgen::bpl(std::string_view sv) { (*output) << "\tbpl\t" << sv << "\n"; }

void Instgen::ldr(Reg r, std::string_view sv) {
    if (!(r.valid() and not r.is_float))
        exit(58);
    (*output) << "\tldr\t" << r << ", " << sv << "\n";
}

void Instgen::movgt(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(59);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(60);
    (*output) << "\tmovgt\t" << r << ", #" << i << "\n";
}
void Instgen::movlt(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(61);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(62);
    (*output) << "\tmovlt\t" << r << ", #" << i << "\n";
}
void Instgen::movge(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(63);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(64);
    (*output) << "\tmovge\t" << r << ", #" << i << "\n";
}
void Instgen::movle(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(65);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(66);
    (*output) << "\tmovle\t" << r << ", #" << i << "\n";
}
void Instgen::movne(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(67);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(68);
    (*output) << "\tmovne\t" << r << ", #" << i << "\n";
}
void Instgen::moveq(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(69);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    if (!(ui < Codegen::imm16 or nui < Codegen::imm8))
        exit(70);
    (*output) << "\tmoveq\t" << r << ", #" << i << "\n";
}
void Instgen::movmi(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(71);
    // assert(i < Codegen::imm16 and -i < Codegen::imm8);
    (*output) << "\tmovmi\t" << r << ", #" << i << "\n";
}
void Instgen::movpl(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(73);
    // assert(i < Codegen::imm16 and -i < Codegen::imm8);
    (*output) << "\tmovpl\t" << r << ", #" << i << "\n";
}
void Instgen::movls(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(75);
    // assert(i < Codegen::imm16 and -i < Codegen::imm8);
    (*output) << "\tmovls\t" << r << ", #" << i << "\n";
}
void Instgen::movhi(Reg r, int i) {
    if (!(r.valid() and not r.is_float))
        exit(77);
    // assert(i < Codegen::imm16 and -i < Codegen::imm8);
    (*output) << "\tmovhi\t" << r << ", #" << i << "\n";
}

void Instgen::str(Reg r, Reg base, int offset, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(79);
    if (!(base.valid() and not base.is_float))
        exit(80);

    if (offset != 0) {
        if (offset < 4096 and offset > -255)
            (*output) << "\tstr" << cond << "\t" << r << ", [" << base << ", #" << offset << "]\n";
        else {
            std::set temp_pool{Codegen::temp_lhs_reg, Codegen::temp_rhs_reg, Codegen::temp_out_reg};
            temp_pool.erase(r);
            temp_pool.erase(base);
            auto temp = *temp_pool.begin();
            Codegen::move(temp, offset, cond);
            str(r, base, temp, cond);
        }
    } else
        (*output) << "\tstr" << cond << "\t" << r << ", [" << base << "]\n";
}

void Instgen::str(Reg r, Reg base, Reg offset, std::string_view cond) {
    assert(r.valid() and not r.is_float);
    assert(base.valid() and not base.is_float);
    assert(offset.valid() and not offset.is_float);
    (*output) << "\tstr" << cond << "\t" << r << ", [" << base << ", " << offset << "]\n";
}

void Instgen::ldr(Reg r, Reg base, Reg offset, std::string_view cond) {
    assert(r.valid() and not r.is_float);
    assert(base.valid() and not base.is_float);
    assert(offset.valid() and not offset.is_float);
    (*output) << "\tldr" << cond << "\t" << r << ", [" << base << ", " << offset << "]\n";
}

void Instgen::ldr(Reg r, Reg base, int offset, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(82);
    if (!(base.valid() and not base.is_float))
        exit(83);
    if (r.is_float) {  // how do we get here?
        Instgen::vldr(r, base, offset, cond);
        return;
    }
    if (offset != 0) {
        if (offset < 4096 and offset > -255)
            (*output) << "\tldr" << cond << "\t" << r << ", [" << base << ", #" << offset << "]\n";
        else {
            std::set temp_pool{Codegen::temp_lhs_reg, Codegen::temp_rhs_reg, Codegen::temp_out_reg};
            temp_pool.erase(r);
            temp_pool.erase(base);
            auto temp = *temp_pool.begin();
            Codegen::move(temp, offset, cond);
            ldr(r, base, temp, cond);
        }
    } else
        (*output) << "\tldr" << cond << "\t" << r << ", [" << base << "]\n";
}

void Instgen::adrl(Reg r, label l, std::string_view cond) {
    if (!(r.valid() and not r.is_float))
        exit(85);
    (*output) << "\tadrl" << cond << "\t" << r << ", " << l << "\n ";
}

void Instgen::adr(Reg r, std::string_view label) {
    (*output) << "\tadr\t" << r << ", " << label << "\n";
}

void Instgen::vadd_f32(Reg r1, Reg r2, Reg r3) {
    if (!(r1.valid() and r1.is_float))
        exit(86);
    if (!(r2.valid() and r2.is_float))
        exit(87);
    if (!(r3.valid() and r3.is_float))
        exit(88);
    (*output) << "\tvadd.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vsub_f32(Reg r1, Reg r2, Reg r3) {
    if (!(r1.valid() and r1.is_float))
        exit(89);
    if (!(r2.valid() and r2.is_float))
        exit(90);
    if (!(r3.valid() and r3.is_float))
        exit(91);
    (*output) << "\tvsub.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vmul_f32(Reg r1, Reg r2, Reg r3) {
    if (!(r1.valid() and r1.is_float))
        exit(92);
    if (!(r2.valid() and r2.is_float))
        exit(93);
    if (!(r3.valid() and r3.is_float))
        exit(94);
    (*output) << "\tvmul.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vdiv_f32(Reg r1, Reg r2, Reg r3) {
    if (!(r1.valid() and r1.is_float))
        exit(95);
    if (!(r2.valid() and r2.is_float))
        exit(96);
    if (!(r3.valid() and r3.is_float))
        exit(97);
    (*output) << "\tvdiv.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}

void Instgen::vstr(Reg fr, Reg base, int offset, std::string_view cond) {
    if (!(fr.valid() and fr.is_float))
        exit(98);
    if (!(base.valid() and not base.is_float))
        exit(99);
    if (offset < 1020 and offset > -1020)
        (*output) << "\tvstr" << cond << "\t" << fr << ", [" << base << ", #" << offset << "]\n";
    else {
        Codegen::move(Codegen::temp_rhs_reg, offset, cond);
        Instgen::add(Codegen::temp_rhs_reg, Codegen::temp_rhs_reg, base);
        (*output) << "\tvstr" << cond << "\t" << fr << ", [" << Codegen::temp_rhs_reg << "]\n";
    }
}

void Instgen::vstr(Reg fr, Reg base, std::string_view cond) {
    if (!(fr.valid() and fr.is_float))
        exit(100);
    if (!(base.valid() and not base.is_float))
        exit(101);
    (*output) << "\tvstr" << cond << "\t" << fr << ", [" << base << "]\n";
}

void Instgen::vldr(Reg fr, Reg base, int offset, std::string_view cond) {
    if (!(fr.valid() and fr.is_float))
        exit(102);
    if (!(base.valid() and not base.is_float))
        exit(103);
    if (offset < 1020 and offset > -1020)
        (*output) << "\tvldr" << cond << "\t" << fr << ", [" << base << ", #" << offset << "]\n";
    else {
        Codegen::move(Codegen::temp_rhs_reg, offset, cond);
        Instgen::add(Codegen::temp_rhs_reg, Codegen::temp_rhs_reg, base);
        (*output) << "\tvldr" << cond << "\t" << fr << ", [" << Codegen::temp_rhs_reg << "]\n";
    }
}
void Instgen::vldr(Reg fr, std::string_view sv) {
    if (!(fr.valid() and fr.is_float))
        exit(104);
    (*output) << "\tvldr\t" << fr << ", " << sv << "\n";
}

void Instgen::vldr(Reg fr, Reg base, std::string_view cond) {
    if (!(fr.valid() and fr.is_float))
        exit(105);
    (*output) << "\tvldr" << cond << "\t" << fr << ", [" << base << "]\n";
}

void Instgen::vcvt_s32_f32(Reg r1, Reg r2) {
    if (!(r1.valid() and r1.is_float))
        exit(106);
    if (!(r2.valid() and r2.is_float))
        exit(107);
    (*output) << "\tvcvt.s32.f32\t" << r1 << ", " << r2 << "\n";
}
void Instgen::vcvt_f32_s32(Reg r1, Reg r2) {
    if (!(r1.valid() and r1.is_float))
        exit(108);
    if (!(r2.valid() and r2.is_float))
        exit(109);
    (*output) << "\tvcvt.f32.s32\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vcmp_f32(Reg r1, Reg r2) {
    if (!(r1.valid() and r1.is_float))
        exit(110);
    if (!(r2.valid() and r2.is_float))
        exit(111);
    (*output) << "\tvcmp.f32\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vmov(Reg r1, Reg r2, std::string_view cond) {
    if (!(r1.valid()))
        exit(112);
    if (!(r2.valid()))
        exit(113);
    if (!(r1.is_float or r2.is_float))
        exit(114);
    if (r1 != r2)
        (*output) << "\tvmov" << cond << "\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vpush(std::vector<Reg> regs) {
    for (auto r : regs)
        if (!(r.valid() and r.is_float))
            exit(115);
    if (regs.empty())
        return;
    (*output) << "\tvpush\t{";
    (*output) << regs.front();
    for (auto it = std::next(regs.begin()); it != regs.end(); ++it)
        (*output) << ", " << *it;
    (*output) << "}\n";
}

void Instgen::vpop(std::vector<Reg> regs) {
    for (auto r : regs)
        if (!(r.valid() and r.is_float))
            exit(116);
    if (regs.empty())
        return;
    (*output) << "\tvpop\t{";
    (*output) << regs.front();
    for (auto it = std::next(regs.begin()); it != regs.end(); ++it)
        (*output) << ", " << *it;
    (*output) << "}\n";
}

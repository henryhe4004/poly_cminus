#include "codegen.hh"
#include "instgen.hh"
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

// void Instgen::push(std::vector<Reg> args) {
//     for (auto arg : args)
//         assert(arg.valid() and not arg.is_float);
//     if (args.empty())
//         return;
//     (*output) << "\tpush\t{";
//     (*output) << *args.begin();
//     for (auto it = std::next(args.begin()); it != args.end(); ++it) {
//         (*output) << ", " << *it;
//     }
//     (*output) << "}\n";
// }

// void Instgen::push_caller_saved() { (*output) << "\tpush\t{r0-r3}\n"; }

// void Instgen::pop_caller_saved() { (*output) << "\tpop\t{r0-r3}\n"; }

// template <typename... Args>
// void Instgen::pop(Args... args) {
//     (*output) << "\tpop\t"
//               << "{" << (... << args) << "}\n";
// }

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

// void Instgen::pop(std::vector<Reg> args) {
//     for (auto arg : args)
//         assert(arg.valid() and not arg.is_float);
//     if (args.empty())
//         return;
//     (*output) << "\tpop\t{";
//     (*output) << *args.begin();
//     for (auto it = std::next(args.begin()); it != args.end(); ++it) {
//         (*output) << ", " << *it;
//     }
//     (*output) << "}\n";
// }

void Instgen::bl(std::string_view sv) { (*output) << "\tbl\t" << sv << "\n"; }

template <typename... Args>
void assert_regs(Args... args) {}

// void Instgen::mov(Reg r1, Reg r2, std::string_view cond) {
//     assert(r1.valid() and not r1.is_float);
//     assert(r2.valid() and not r2.is_float);
//     if (r1 != r2)
//         (*output) << "\tmov" << cond << "\t" << r1 << ", " << r2 << "\n";
// }

// void Instgen::mov(Reg r, int i, std::string_view cond) {
//     assert(r.valid() and not r.is_float);
//     unsigned int ui = static_cast<unsigned int>(i);
//     unsigned int nui = static_cast<unsigned int>(-i);
//     assert(ui < Codegen::imm16 or nui < Codegen::imm8);
//     (*output) << "\tmov" << cond << "\t" << r << ", #" << i << "\n";
// }

// void Instgen::movt(Reg r, int i, std::string_view cond) {
//     assert(r.valid() and not r.is_float);
//     (*output) << "\tmovt" << cond << "\t" << r << ", #" << i << "\n";
// }

// void Instgen::movw(Reg r, int i, std::string_view cond) {
//     assert(r.valid() and not r.is_float);
//     (*output) << "\tmovw" << cond << "\t" << r << ", #" << i << "\n";
// }

void Instgen::sub(Reg r1, Reg r2, int n, std::string_view cond) {
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\tsub" << cond << "\t" << r1 << ", " << r2 << ", #" << n << "\n";
}

void Instgen::add(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::add(Reg dest, Reg r1, Reg r2, std::string_view cond, std::string_view suffix) {
    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", " << r2 << ", " << suffix << "\n";
}

void Instgen::sub(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\tsub" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}
void Instgen::rsub(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\trsb" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::mul(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
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
    assert(r1.valid() and not r1.is_float);
    assert(r1.valid() and not r2.is_float);
    (*output) << "\tcmp\t" << r1 << ", " << r2 << "\n";
}
void Instgen::tst(Reg r1, int n) { (*output) << "\ttst\t" << r1 << ", #" << n << "\n"; }

void Instgen::add(Reg dest, Reg r1, int imm, std::string_view cond) {
    assert(r1.valid() and not dest.is_float);
    assert(not r1.is_float);

    (*output) << "\tadd" << cond << "\t" << dest << ", " << r1 << ", #" << imm << "\n";
}
void Instgen::lsl(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\tlsl" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}
void Instgen::lsr(Reg dest, Reg r1, Reg r2, std::string_view cond) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float);
    (*output) << "\tlsr" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::shift(Reg r1, Reg r2) {
    assert (r2.shift_n>0);
    assert (r1!=Reg::ID::zero);
    switch (r2.op) {
        case shift_type_t::LSL:
            (*output) << "\tslli.d\t" << r1 << ", " << r2 << ", " <<r2.shift_n << "\n";
            break;
        case shift_type_t::LSR:
            (*output) << "\tsrli.d\t" << r1 << ", " << r2 << ", " <<r2.shift_n << "\n";
            break;
        case shift_type_t::ASR:
            (*output) << "\tsrai.d\t" << r1 << ", " << r2 << ", " <<r2.shift_n << "\n";
            break;
        default:
            LOG_ERROR << "shift type error";
            exit(-1);
    }
}

// void Instgen::asr(Reg dest, Reg r1, Reg r2, std::string_view cond) {
//     assert(dest.valid() and not dest.is_float);
//     assert(r1.valid() and not r1.is_float);
//     assert(r2.valid() and not r2.is_float);
//     (*output) << "\tasr" << cond << "\t" << dest << ", " << r1 << ", " << r2 << "\n";
// }


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
// void Instgen::b(std::string_view sv) { (*output) << "\tb\t" << sv << "\n"; }
void Instgen::beq(std::string_view sv) { (*output) << "\tbeq\t" << sv << "\n"; }
void Instgen::bne(std::string_view sv) { (*output) << "\tbne\t" << sv << "\n"; }
void Instgen::bgt(std::string_view sv) { (*output) << "\tbgt\t" << sv << "\n"; }
void Instgen::bge(std::string_view sv) { (*output) << "\tbge\t" << sv << "\n"; }
void Instgen::blt(std::string_view sv) { (*output) << "\tblt\t" << sv << "\n"; }
void Instgen::ble(std::string_view sv) { (*output) << "\tble\t" << sv << "\n"; }

void Instgen::bhi(std::string_view sv) { (*output) << "\tbhi\t" << sv << "\n"; }
void Instgen::bpl(std::string_view sv) { (*output) << "\tbpl\t" << sv << "\n"; }

void Instgen::ldr(Reg r, std::string_view sv) {
    if (sv != "")
        LOG_WARNING << "ldr " << r << ", " << sv << " is not supported yet";
    (*output) << "\tldr\t" << r << ", " << sv << "\n";
}

void Instgen::movgt(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmovgt\t" << r << ", #" << i << "\n";
}
void Instgen::movlt(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmovlt\t" << r << ", #" << i << "\n";
}
void Instgen::movge(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmovge\t" << r << ", #" << i << "\n";
}
void Instgen::movle(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmovle\t" << r << ", #" << i << "\n";
}
void Instgen::movne(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmovne\t" << r << ", #" << i << "\n";
}
void Instgen::moveq(Reg r, int i) {
    assert(r.valid() and not r.is_float);
    unsigned int ui = static_cast<unsigned int>(i);
    unsigned int nui = static_cast<unsigned int>(-i);
    assert(ui < Codegen::imm16 or nui < Codegen::imm8);
    (*output) << "\tmoveq\t" << r << ", #" << i << "\n";
}
void Instgen::movmi(Reg r, int i) {
    assert(r.valid() and not r.is_float);

    (*output) << "\tmovmi\t" << r << ", #" << i << "\n";
}
void Instgen::movpl(Reg r, int i) {
    assert(r.valid() and not r.is_float);

    (*output) << "\tmovpl\t" << r << ", #" << i << "\n";
}
void Instgen::movls(Reg r, int i) {
    assert(r.valid() and not r.is_float);

    (*output) << "\tmovls\t" << r << ", #" << i << "\n";
}
void Instgen::movhi(Reg r, int i) {
    assert(r.valid() and not r.is_float);

    (*output) << "\tmovhi\t" << r << ", #" << i << "\n";
}

void Instgen::str(Reg r, Reg base, int offset, std::string_view cond) {
    assert(r.valid() and not r.is_float);
    assert(base.valid() and not base.is_float);

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
    if (cond != "")
        LOG_WARNING << "ldr " << r << ", [" << base << ", " << offset << "] " << cond << " is not supported\n";
    assert(r.valid() and not r.is_float);
    assert(base.valid() and not base.is_float);
    assert(offset.valid() and not offset.is_float);
    (*output) << "\tldr" << cond << "\t" << r << ", [" << base << ", " << offset << "]\n";
}

void Instgen::ldr(Reg r, Reg base, int offset, std::string_view cond) {
    assert(r.valid() and not r.is_float);
    assert(base.valid() and not base.is_float);

    if (cond != "")
        LOG_WARNING << "ldr " << r << ", [" << base << ", " << offset << "] " << cond << " is not supported";
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
    assert(r.valid() and not r.is_float);
    (*output) << "\tadrl" << cond << "\t" << r << ", " << l << "\n ";
}

void Instgen::vadd_f32(Reg r1, Reg r2, Reg r3) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    assert(r3.valid() and r3.is_float);
    (*output) << "\tvadd.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vsub_f32(Reg r1, Reg r2, Reg r3) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    assert(r3.valid() and r3.is_float);
    (*output) << "\tvsub.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vmul_f32(Reg r1, Reg r2, Reg r3) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    assert(r3.valid() and r3.is_float);
    (*output) << "\tvmul.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}
void Instgen::vdiv_f32(Reg r1, Reg r2, Reg r3) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    assert(r3.valid() and r3.is_float);
    (*output) << "\tvdiv.f32\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}

void Instgen::vstr(Reg fr, Reg base, int offset, std::string_view cond) {
    assert(fr.valid() and fr.is_float);
    assert(base.valid() and not base.is_float);
    if (offset < 1020 and offset > -1020)
        (*output) << "\tvstr" << cond << "\t" << fr << ", [" << base << ", #" << offset << "]\n";
    else {
        Codegen::move(Codegen::temp_rhs_reg, offset, cond);
        Instgen::add(Codegen::temp_rhs_reg, Codegen::temp_rhs_reg, base);
        (*output) << "\tvstr" << cond << "\t" << fr << ", [" << Codegen::temp_rhs_reg << "]\n";
    }
}

void Instgen::vstr(Reg fr, Reg base, std::string_view cond) {
    assert(fr.valid() and fr.is_float);
    assert(base.valid() and not base.is_float);
    (*output) << "\tvstr" << cond << "\t" << fr << ", [" << base << "]\n";
}

void Instgen::vldr(Reg fr, Reg base, int offset, std::string_view cond) {
    assert(fr.valid() and fr.is_float);
    assert(base.valid() and not base.is_float);
    if (offset < 1020 and offset > -1020)
        (*output) << "\tvldr" << cond << "\t" << fr << ", [" << base << ", #" << offset << "]\n";
    else {
        Codegen::move(Codegen::temp_rhs_reg, offset, cond);
        Instgen::add(Codegen::temp_rhs_reg, Codegen::temp_rhs_reg, base);
        (*output) << "\tvldr" << cond << "\t" << fr << ", [" << Codegen::temp_rhs_reg << "]\n";
    }
}
void Instgen::vldr(Reg fr, std::string_view sv) {
    assert(fr.valid() and fr.is_float);
    (*output) << "\tvldr\t" << fr << ", " << sv << "\n";
}

void Instgen::vldr(Reg fr, Reg base, std::string_view cond) {
    assert(fr.valid() and fr.is_float);
    (*output) << "\tvldr" << cond << "\t" << fr << ", [" << base << "]\n";
}

void Instgen::vcvt_s32_f32(Reg r1, Reg r2) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    (*output) << "\tvcvt.s32.f32\t" << r1 << ", " << r2 << "\n";
}
void Instgen::vcvt_f32_s32(Reg r1, Reg r2) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    (*output) << "\tvcvt.f32.s32\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vcmp_f32(Reg r1, Reg r2) {
    assert(r1.valid() and r1.is_float);
    assert(r2.valid() and r2.is_float);
    (*output) << "\tvcmp.f32\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vmov(Reg r1, Reg r2, std::string_view cond) {
    assert(r1.valid());
    assert(r2.valid());
    assert(r1.is_float or r2.is_float);
    if (r1 != r2)
        (*output) << "\tvmov" << cond << "\t" << r1 << ", " << r2 << "\n";
}

void Instgen::vpush(std::vector<Reg> regs) {
    for (auto r : regs)
        assert(r.valid() and r.is_float);
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
        assert(r.valid() and r.is_float);
    if (regs.empty())
        return;
    (*output) << "\tvpop\t{";
    (*output) << regs.front();
    for (auto it = std::next(regs.begin()); it != regs.end(); ++it)
        (*output) << ", " << *it;
    (*output) << "}\n";
}

// LoongArch
void Instgen::or_(Reg r1, Reg r2, Reg r3) { (*output) << "\tor\t" << r1 << ", " << r2 << ", " << r3 << "\n"; }

// ori
void Instgen::ori(Reg r1, Reg r2, int imm) { (*output) << "\tori\t" << r1 << ", " << r2 << ", " << imm << "\n"; }

// xori
void Instgen::xori(Reg r1, Reg r2, int imm) { (*output) << "\txori\t" << r1 << ", " << r2 << ", " << imm << "\n"; }

// lu12i.w
void Instgen::lu12i_w(Reg r1, int imm) { (*output) << "\tlu12i.w\t" << r1 << ", " << imm << "\n"; }

// lu12i.d
void Instgen::lu12i_d(Reg r1, int imm) { (*output) << "\tlu12i.d\t" << r1 << ", " << imm << "\n"; }

// addi.w
void Instgen::addi_w(Reg r1, Reg r2, int imm) {
    assert(r1 != Reg::ID::zero);
    if (r1 == r2 and imm == 0)
        return;
    (*output) << "\taddi.w\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// addi.d
void Instgen::addi_d(Reg r1, Reg r2, int imm) {
    assert(r1 != Reg::ID::zero);
    if (r1 == r2 and imm == 0)
        return;
    (*output) << "\taddi.d\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// mov, use or to implement
void Instgen::mov(Reg r1, Reg r2, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in mov";
    if (r2.id == 4) {
        LOG_DEBUG << "mov " << r1 << ", " << r2;
        // exit(1);
    }
    if (r1 != r2)
        Instgen::or_(r1, r2, Reg::ID::zero);
}

// mov, use addi_w to implement
void Instgen::mov(Reg r1, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in mov";

    if (imm < 2048 and imm >= -2048)
        Instgen::addi_d(r1, Reg::ID::zero, imm);
    else if (imm < 1 << 12 and imm > 0)
        Instgen::ori(r1, Reg::ID::zero, imm);
    else {
        if (imm < 0)
            LOG_DEBUG << "imm is negative";
        int upper20 = imm >> 12;
        int lower12 = imm & 0xfff;
        Instgen::lu12i_w(r1, upper20);
        Instgen::ori(r1, r1, lower12);
    }
}

// add.w
void Instgen::add_w(Reg r1, Reg r2, Reg r3) {
    assert(r1 != Reg::ID::zero);
    (*output) << "\tadd.w\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}

// add.d
void Instgen::add_d(Reg r1, Reg r2, Reg r3) {
    assert(r1 != Reg::ID::zero);
    (*output) << "\tadd.d\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}


// slt
void Instgen::slt(Reg r1, Reg r2, Reg r3) {
    assert(r1 != Reg::ID::zero);
    (*output) << "\tslt\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}

//slti
void Instgen::slti(Reg r1,Reg r2,int imm){
    assert(r1 != Reg::ID::zero);
    if (r1 == r2 and imm == 0)
        return;
    (*output) << "\tslti\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// sub.w
void Instgen::sub_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tsub.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; \
}

// sub.d
void Instgen::sub_d(Reg r1, Reg r2, Reg r3) {
    assert(r1 != Reg::ID::zero);
    (*output) << "\tsub.d\t" << r1 << ", " << r2 << ", " << r3 << "\n";
}

// mul.w
void Instgen::mul_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tmul.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}
// mul.d
void Instgen::mul_d(Reg r1,Reg r2,Reg r3){
    (*output) << "\tmul.d\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// div.w
void Instgen::div_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tdiv.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// div.d
void Instgen::div_d(Reg r1,Reg r2,Reg r3){
    (*output) << "\tdiv.d\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}


// mod.w
void Instgen::mod_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tmod.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}
// mod.d
void Instgen::mod_d(Reg r1,Reg r2,Reg r3){
    (*output) << "\tmod.d\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// st.w
void Instgen::st_w(Reg r1, Reg r2, int imm) { (*output) << "\tst.w\t" << r1 << ", " << r2 << ", " << imm << "\n"; }

// st.d
void Instgen::st_d(Reg r1, Reg r2, int imm) {
    if (-2048 <= imm and imm < 2048)
        (*output) << "\tst.d\t" << r1 << ", " << r2 << ", " << imm << "\n";
    else {
        Instgen::mov(Codegen::temp_lhs_reg, imm);
        Instgen::stx_d(r1, r2, Codegen::temp_lhs_reg);
    }
}
// stx.w
void Instgen::stx_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tstx.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// stx.d
void Instgen::stx_d(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tstx.d\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// ld.w
void Instgen::ld_w(Reg r1, Reg r2, int imm) { 
    (*output) << "\tld.w\t" << r1 << ", " << r2 << ", " << imm << "\n"; 
}
// ld.d
void Instgen::ld_d(Reg r1, Reg r2, int imm) {
    if (-2048 <= imm and imm < 2048)
        (*output) << "\tld.d\t" << r1 << ", " << r2 << ", " << imm << "\n";
    else {
        Instgen::mov(Codegen::temp_lhs_reg, imm);
        Instgen::ldx_d(r1, r2, Codegen::temp_lhs_reg);
    }
}
// ldx_w
void Instgen::ldx_w(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tldx.w\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// ldx.d
void Instgen::ldx_d(Reg r1, Reg r2, Reg r3) { 
    (*output) << "\tldx.d\t" << r1 << ", " << r2 << ", " << r3 << "\n"; 
}

// st.w
void Instgen::st_w(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in st.w";
    Instgen::st_w(r1, r2, imm);
}

// st.d
void Instgen::st_d(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (imm % 8 != 0)
        LOG_WARNING << "st.d " << r1 << ", " << r2 << ", " << imm << " is not 8-byte aligned";
    if (cond != "")
        LOG_WARNING << cond << " is not supported in st.d";
    Instgen::st_d(r1, r2, imm);
}




void Instgen::ld_d(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in ld.d";
    Instgen::ld_d(r1, r2, imm);
}

void Instgen::ld_w(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in ld.w";
    Instgen::ld_w(r1, r2, imm);
}

// 	la.local
void Instgen::la_local(Reg r1, label l) { 
    (*output) << "\tla.local\t" << r1 << ", " << l << "\n"; 
}

// ldptr.w
void Instgen::ldptr_w(Reg r1, Reg r2, int imm) {
    (*output) << "\tldptr.w\t" << r1 << ", " << r2 << ", " << imm << "\n";
}
// ldptr.d
void Instgen::ldptr_d(Reg r1, Reg r2 ,int imm){
    (*output) << "\tldptr.d\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// jr
void Instgen::jr(Reg r1) { (*output) << "\tjr\t" << r1 << "\n"; }

void Instgen::beq(Reg r1, Reg r2, std::string_view label) {
    (*output) << "\tbeq\t" << r1 << ", " << r2 << ", " << label << "\n";
}

void Instgen::bne(Reg r1, Reg r2, std::string_view label) {
    (*output) << "\tbne\t" << r1 << ", " << r2 << ", " << label << "\n";
}

void Instgen::bge(Reg r1, Reg r2, std::string_view label) {
    (*output) << "\tbge\t" << r1 << ", " << r2 << ", " << label << "\n";
}

void Instgen::blt(Reg r1, Reg r2, std::string_view label) {
    (*output) << "\tblt\t" << r1 << ", " << r2 << ", " << label << "\n";
}

// bnez
void Instgen::bnez(Reg r1, std::string_view label) { (*output) << "\tbnez\t" << r1 << ", " << label << "\n"; }

// beqz
void Instgen::beqz(Reg r1, std::string_view label) { (*output) << "\tbeqz\t" << r1 << ", " << label << "\n"; }

// b	.L3
void Instgen::b(std::string_view label) { (*output) << "\tb\t" << label << "\n"; }

// movgr2fr.w
void Instgen::movgr2fr_w(Reg r1, Reg r2) { (*output) << "\tmovgr2fr.w\t" << r1 << ", " << r2 << "\n"; }

// movfr2gr.s
void Instgen::movfr2gr_s(Reg r1, Reg r2) { (*output) << "\tmovfr2gr.s\t" << r1 << ", " << r2 << "\n"; }

// fcmp.slt.s
void Instgen::fcmp_slt_s(std::string_view cc, Reg r1, Reg r2) {
    (*output) << "\tfcmp.slt.s\t" << cc << ", " << r1 << ", " << r2 << "\n";
}

// fcmp.sune.s
void Instgen::fcmp_sune_s(std::string_view cc, Reg r1, Reg r2) {
    (*output) << "\tfcmp.sune.s\t" << cc << ", " << r1 << ", " << r2 << "\n";
}

// bceqz
void Instgen::bceqz(std::string_view cc, std::string_view sv) { (*output) << "\tbceqz\t" << cc << ", " << sv << "\n"; }

// bcnez
void Instgen::bcnez(std::string_view cc, std::string_view sv) { (*output) << "\tbcnez\t" << cc << ", " << sv << "\n"; }

// fadd.s
void Instgen::fadd_s(Reg r1, Reg r2, Reg r3) { (*output) << "\tfadd.s\t" << r1 << ", " << r2 << ", " << r3 << "\n"; }

// fsub.s
void Instgen::fsub_s(Reg r1, Reg r2, Reg r3) { (*output) << "\tfsub.s\t" << r1 << ", " << r2 << ", " << r3 << "\n"; }

// fmul.s
void Instgen::fmul_s(Reg r1, Reg r2, Reg r3) { (*output) << "\tfmul.s\t" << r1 << ", " << r2 << ", " << r3 << "\n"; }

// fdiv.s
void Instgen::fdiv_s(Reg r1, Reg r2, Reg r3) { (*output) << "\tfdiv.s\t" << r1 << ", " << r2 << ", " << r3 << "\n"; }

// ftintrz.w.s
void Instgen::ftintrz_w_s(Reg r1, Reg r2) { (*output) << "\tftintrz.w.s\t" << r1 << ", " << r2 << "\n"; }

// ffint.s.w
void Instgen::ffint_s_w(Reg r1, Reg r2) { (*output) << "\tffint.s.w\t" << r1 << ", " << r2 << "\n"; }

// fld.s
void Instgen::fld_s(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in fld.s";
    (*output) << "\tfld.s\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// fst.s
void Instgen::fst_s(Reg r1, Reg r2, int imm, std::string_view cond) {
    if (cond != "")
        LOG_WARNING << cond << " is not supported in fst.s";
    (*output) << "\tfst.s\t" << r1 << ", " << r2 << ", " << imm << "\n";
}

// fmov.s
void Instgen::fmov_s(Reg r1, Reg r2) { (*output) << "\tfmov.s\t" << r1 << ", " << r2 << "\n"; }

// fmov
void Instgen::reg_mov(Reg r1, Reg r2, std::string_view cond) {
    if (r1 == r2)
        return;
    if (cond != "")
        LOG_WARNING << cond << " is not supported in mov";

    if (r1.is_float and r2.is_float)
        Instgen::fmov_s(r1, r2);
    else if (r1.is_float and not r2.is_float)
        Instgen::movgr2fr_w(r1, r2);
    else if (not r1.is_float and r2.is_float)
        Instgen::movfr2gr_s(r1, r2);
    else
        Instgen::mov(r1, r2);
}
//andi
void Instgen::andi(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tandi\t" << dest << ", " << r1 << ", " << imm << "\n";
}
//and_
void Instgen::and_(Reg dest, Reg r1, Reg r2) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    assert(r2.valid() and not r2.is_float) ;
    (*output) << "\tand\t" << dest << ", " << r1 << ", " << r2 << "\n";
}

void Instgen::maskeqz(Reg r1,Reg r2,Reg r3){
    (*output) << "\tmaskeqz\t" << r1 << ", " << r2 << ", " << r3 <<"\n";
}

void Instgen::masknez(Reg r1,Reg r2,Reg r3){
    (*output) << "\tmasknez\t" << r1 << ", " << r2 << ", " << r3 <<"\n";
}

void Instgen::slli_w(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tslli.w\t" << dest << ", " << r1 << ", " << imm << "\n";
}

void Instgen::slli_d(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tslli.d\t" << dest << ", " << r1 << ", " << imm << "\n";
}

void Instgen::srli_w(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tsrli.w\t" << dest << ", " << r1 << ", " << imm << "\n";
}

void Instgen::srli_d(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tsrli.d\t" << dest << ", " << r1 << ", " << imm << "\n";
}


void Instgen::srai_w(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tsrai.w\t" << dest << ", " << r1 << ", " << imm<< "\n";
}

void Instgen::srai_d(Reg dest, Reg r1, int imm) {
    assert(dest.valid() and not dest.is_float);
    assert(r1.valid() and not r1.is_float);
    (*output) << "\tsrai.w\t" << dest << ", " << r1 << ", " << imm<< "\n";
}

// void Instgen::bceqz(Reg dest,int imm){
//      (*output) << "\tbceqz\t" << dest << ", " << imm<< "\n";
// }

void Instgen::li_w(Reg dest,int imm){
    (*output) << "\tli.w\t" << dest << ", " << imm<< "\n";
}

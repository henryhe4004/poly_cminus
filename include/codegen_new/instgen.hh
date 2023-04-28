#pragma once
#include "val.hh"

#include <fstream>
#include <set>
#include <vector>

using std::literals::string_view_literals::operator""sv;

class Codegen;
class Instgen {
    // TODO: reuse code
    friend class Codegen;
    struct label {
        std::string_view lbl;
        int offset;
        friend std::ostream &operator<<(std::ostream &os, const label &l) {
            if (l.offset == 0)
                return os << l.lbl;
            else
                return os << l.lbl << "+" << l.offset;
        }
    };
    struct mem {
        Reg reg;
        int offset;
        friend std::ostream &operator<<(std::ostream &os, const mem &l) { return os << l.reg << "+" << l.offset; }
    };

  public:
    // Instgen(std::ofstream &output) : output(output) {}
    template <typename... Args>
    static void push(Args... args);
    static void push(std::vector<Reg> args);
    // static void push(std::set<Reg> args);
    static void push_caller_saved();
    template <typename... Args>
    static void pop(Args... args);
    static void pop(std::vector<Reg> args);
    // static void pop(std::set<Reg> args);
    static void pop_caller_saved();

    static void add(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void add(Reg, Reg, Reg, std::string_view cond, std::string_view suffix);
    static void add(Reg, Reg, int, std::string_view cond = ""sv);
    static void sub(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void sub(Reg, Reg, int, std::string_view cond = ""sv);
    static void rsub(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void mul(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void sdiv(Reg, Reg, Reg);
    static void smmul(Reg, Reg, Reg);

    static void cmp(Reg, Reg);
    static void tst(Reg, int);
    static void lsl(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void lsr(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void asr(Reg, Reg, Reg, std::string_view cond = ""sv);
    
    static void asr(Reg, Reg, int);
   
    static void bic(Reg, Reg, int);
    static void lsr(Reg, Reg, int);
    static void lsl(Reg, Reg, int);
    static void mla(Reg, Reg, Reg, Reg);
    static void mls(Reg, Reg, Reg, Reg);
    static void bl(std::string_view sv);
    static void bx(Reg);
    static void b(std::string_view sv);
    static void ite(std::string_view sv);
    static void shift(Reg r1, Reg r2);
    static void beq(std::string_view sv);
    static void bne(std::string_view sv);
    static void bgt(std::string_view sv);
    static void bge(std::string_view sv);
    static void blt(std::string_view sv);
    static void ble(std::string_view sv);

    static void bhi(std::string_view sv);
    static void bpl(std::string_view sv);

    static void ldr(Reg, std::string_view sv);

    static void mov(Reg, Reg, std::string_view cond = ""sv);
    static void mov(Reg, int, std::string_view cond = ""sv);
    static void movt(Reg, int, std::string_view cond = ""sv);
    static void movw(Reg, int, std::string_view cond = ""sv);
    static void movgt(Reg, int);
    static void movlt(Reg, int);
    static void movle(Reg, int);
    static void movge(Reg, int);
    static void movne(Reg, int);
    static void moveq(Reg, int);
    static void movmi(Reg, int);
    static void movpl(Reg, int);
    static void movls(Reg, int);
    static void movhi(Reg, int);

    static void str(Reg, Reg, int = 0, std::string_view = ""sv);
    static void ldr(Reg, Reg, int = 0, std::string_view = ""sv);
    static void str(Reg, Reg, Reg, std::string_view = ""sv);
    static void ldr(Reg, Reg, Reg, std::string_view = ""sv);
    static void adrl(Reg, label, std::string_view cond = ""sv);

    static void vadd_f32(Reg, Reg, Reg);
    static void vsub_f32(Reg, Reg, Reg);
    static void vmul_f32(Reg, Reg, Reg);
    static void vdiv_f32(Reg, Reg, Reg);

    static void vstr(Reg, Reg, int, std::string_view cond = ""sv);
    static void vstr(Reg, Reg, std::string_view cond = ""sv);
    static void vldr(Reg, Reg, int, std::string_view cond = ""sv);
    static void vldr(Reg, Reg, std::string_view cond = ""sv);
    static void vldr(Reg, std::string_view);

    static void vcvt_s32_f32(Reg, Reg);
    static void vcvt_f32_s32(Reg, Reg);
    static void vcmp_f32(Reg, Reg);

    static void vmov(Reg, Reg, std::string_view cond = ""sv);
    static void vpush(std::vector<Reg>);
    static void vpop(std::vector<Reg>);

    // LoongArch
    static void or_(Reg, Reg, Reg);
    static void ori(Reg, Reg, int);
    static void xori(Reg, Reg, int);
    static void lu12i_w(Reg, int);
    static void lu12i_d(Reg, int);
    static void addi_w(Reg, Reg, int);
    static void addi_d(Reg, Reg, int);
    static void add_w(Reg, Reg, Reg);
    static void add_d(Reg,Reg,Reg);
    static void sub_w(Reg, Reg, Reg);
    static void sub_d(Reg,Reg,Reg);
    static void mul_w(Reg, Reg, Reg);
    static void mul_d(Reg,Reg,Reg);
    static void div_w(Reg, Reg, Reg);
    static void div_d(Reg,Reg,Reg);
    static void mod_w(Reg, Reg, Reg);
    static void mod_d(Reg,Reg,Reg);

    static void slt(Reg, Reg, Reg);
    static void slti(Reg, Reg, int);

    static void st_d(Reg, Reg, int = 0);
    static void st_w(Reg, Reg, int = 0);
    static void ld_d(Reg, Reg, int = 0);
    static void ld_w(Reg, Reg, int = 0);

    static void stx_d(Reg, Reg, Reg);
    static void ldx_d(Reg, Reg, Reg);

    static void stx_w(Reg, Reg, Reg);
    static void ldx_w(Reg,Reg,Reg);

    static void st_d(Reg, Reg, int, std::string_view cond);
    static void st_w(Reg, Reg, int, std::string_view cond);
    static void ld_d(Reg, Reg, int, std::string_view cond);
    static void ld_w(Reg, Reg, int, std::string_view cond);

    static void la_local(Reg, label);
    static void ldptr_w(Reg, Reg, int = 0);
    static void ldptr_d(Reg, Reg, int = 0);
    static void jr(Reg);

    static void beq(Reg, Reg, std::string_view sv);
    static void bne(Reg, Reg, std::string_view sv);
    static void bge(Reg, Reg, std::string_view sv);
    static void blt(Reg, Reg, std::string_view sv);
    static void bnez(Reg, std::string_view sv);
    static void beqz(Reg, std::string_view sv);

    static void fadd_s(Reg, Reg, Reg);
    static void fsub_s(Reg, Reg, Reg);
    static void fmul_s(Reg, Reg, Reg);
    static void fdiv_s(Reg, Reg, Reg);

    static void ftintrz_w_s(Reg, Reg);
    static void ffint_s_w(Reg, Reg);

    static void fld_s(Reg, Reg, int = 0, std::string_view cond = ""sv);
    static void fst_s(Reg, Reg, int = 0, std::string_view cond = ""sv);
    static void fmov_s(Reg, Reg);
    static void movgr2fr_w(Reg, Reg);
    static void movfr2gr_s(Reg, Reg);

    static void fcmp_slt_s(std::string_view cc, Reg, Reg);
    static void fcmp_sune_s(std::string_view cc, Reg, Reg);
    static void bceqz(std::string_view cc, std::string_view sv);
    static void bcnez(std::string_view cc, std::string_view sv);

    static void reg_mov(Reg, Reg, std::string_view cond = ""sv);
    static void maskeqz(Reg,Reg,Reg);
    static void masknez(Reg,Reg,Reg);
    static void and_(Reg, Reg, Reg);
    static void andi(Reg,Reg,int);
    static void srli_w(Reg, Reg, int);
    static void srli_d(Reg,Reg,int);
    static void slli_w(Reg,Reg,int);
    static void slli_d(Reg,Reg,int);
    static void srai_w(Reg,Reg,int);
    static void srai_d(Reg,Reg,int);
    static void bceqz(Reg,int);
    static void li_w(Reg,int);
  private:
    static std::ofstream *output;
    // std::ofstream& output;
};

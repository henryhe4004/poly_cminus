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
        friend std::ostream &operator<<(std::ostream &os, const label &l) { return os << l.lbl << "+" << l.offset; }
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
    static void cmp(Reg, int);
    static void tst(Reg, int);
    static void lsl(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void lsr(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void asr(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void and_(Reg, Reg, Reg, std::string_view cond = ""sv);
    static void asr(Reg, Reg, int);
    static void and_(Reg, Reg, int);
    static void bic(Reg, Reg, int);
    static void lsr(Reg, Reg, int);
    static void lsl(Reg, Reg, int);
    static void mla(Reg, Reg, Reg, Reg);
    static void mls(Reg, Reg, Reg, Reg);
    static void bl(std::string_view sv);
    static void bx(Reg);
    static void b(std::string_view sv);
    static void ite(std::string_view sv);

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
    static void adr(Reg,std::string_view label);
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

  private:
    static std::ofstream *output;
    // std::ofstream& output;
};

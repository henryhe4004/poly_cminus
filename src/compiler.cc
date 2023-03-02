#include "AST.hpp"
#include "passes.hh"
#include "SYSYCBuilder.h"
#include "SyntaxTree.hh"
#include "codegen.hh"
#include "logging.hpp"
#include "parser.hh"
#include "utils.hh"

#include <boost/version.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::string;
using std::literals::string_literals::operator""s;
using std::literals::string_view_literals::operator""sv;
SyntaxTree syntax_tree;
std::string help_text;
void print_usage() {
    // TODO
    cout << "Usage: "
         << "cc"
         << " <input_file_path>" << help_text << endl;
}

void _trigger_opt(bool &val, const char *arg, const char *argv) {
    if (strcmp(arg, argv) == 0)
        val ^= 1;  // 用以在开启 O2 时关闭特定优化
    else if (strlen(argv) > 3 and strcmp(arg, argv + 3) == 0) {
        val = false;  // 使用类似 -no-dead-code 的选项关闭特定优化
        LOG_INFO << "Turn off " << arg << " optimization";
    }
}

template <typename T>
void _add_pass_if(PassManager &pm, bool opt) {
    if (opt)
        pm.add_pass<T>();
}

int main(int argc, char *argv[]) {
    bool default_value = false;
    for (int i = 1; i < argc; ++i)
        if (argv[i] == "-O2"sv) {
            default_value = true;
            break;
        }

    bool silent = false;
    bool emit_llvm = false;
    bool loop_info = false;
    bool dump_graph = false;
    bool loop_sea = false;
    bool func_info = false;
    bool post_dom = false;
    bool debug_ir = false;
    bool dom = false;
    bool ir_check = true;

    bool adce = default_value;
    bool simplify_cfg = default_value;
    bool sccp = default_value;
    bool mem2reg = default_value;
    bool loop_merge = default_value;
    bool loop_inv = default_value;
    bool loop_unroll = default_value;
    bool algebraic_simplification = default_value;
    bool div_inv = default_value;
    bool dead_code = default_value;
    bool gep_elim = default_value;
    bool iv_red = default_value;
    bool inst_simpl = default_value;
    bool lce = default_value;
    bool func_inline = default_value;
    bool useless_eli = default_value;
    bool mul_weaken = default_value;
    bool glo_var_local = default_value;
    bool lower_ir = default_value;
    bool tail_opt = default_value;
    bool dead_global = default_value;
    bool poly = false;
    bool peephole = default_value;
    bool poly_test = false;
    bool adhoc = default_value;
    bool branch_opt= default_value;
    bool transform_select = default_value; //将select的中间代码转换成
    string input_file_path, target_file_path;
    int reg_num = 10;
    if (argc <= 1) {
        print_usage();
        return 1;
    }

#ifndef LOCAL_TEST
    mem2reg = true;
    gep_elim = true;
#endif

    for (int i = 1; i < argc; ++i) {
#define trigger_opt(val, arg) _trigger_opt(val, arg, argv[i]), help_text += " [", help_text += arg, help_text += "]"
        trigger_opt(ir_check, "-ir-check");
        trigger_opt(dom, "-dom");
        trigger_opt(adce, "-adce");
        trigger_opt(debug_ir, "-debug-ir");
        trigger_opt(func_info, "-func-info");
        trigger_opt(post_dom, "-post-dom");
        trigger_opt(silent, "-S");
        trigger_opt(emit_llvm, "-emit-llvm");
        trigger_opt(mem2reg, "-mem2reg");
        trigger_opt(loop_sea, "-loop-sea");
        trigger_opt(loop_merge, "-loop-merge");
        trigger_opt(dump_graph, "-dump-graph");
        trigger_opt(loop_inv, "-loop-inv");
        trigger_opt(loop_unroll, "-loop-unroll");
        trigger_opt(algebraic_simplification, "-alge-simpl");
        trigger_opt(div_inv, "-div-inv");
        trigger_opt(simplify_cfg, "-simplify-cfg");
        trigger_opt(dead_code, "-dead-code");
        trigger_opt(loop_info, "-loop-info");
        trigger_opt(iv_red, "-iv-red");
        trigger_opt(lce, "-lce");
        trigger_opt(func_inline, "-func-inline");
        trigger_opt(useless_eli, "-useless-eli");
        trigger_opt(mul_weaken, "-mul-weaken");
        trigger_opt(gep_elim, "-gep-elim");
        trigger_opt(inst_simpl, "-inst-simpl");
        trigger_opt(lower_ir, "-lower-ir");
        trigger_opt(tail_opt, "-tail-opt");
        trigger_opt(peephole, "-peephole");
        trigger_opt(sccp, "-sccp");
        trigger_opt(dead_global, "-dead-global");
        trigger_opt(glo_var_local, "-glo-local");
        trigger_opt(poly, "-poly");
        trigger_opt(poly_test, "-poly-test");
        trigger_opt(adhoc, "-adhoc");
        trigger_opt(branch_opt, "-branch-opt");
        trigger_opt(transform_select,"-transform-select");
#undef trigger_opt
        if (argv[i] == "--help"sv || argv[i] == "-h"sv) {
            print_usage();
            return 0;
        } else if (argv[i] == "-o"sv) {
            if (target_file_path.empty() && i + 1 < argc) {
                target_file_path = argv[i + 1];
                i += 1;
            } else {
                print_usage();
                return 1;
            }
        } else if (argv[i] == "-r"sv or argv[i] == "--regs"sv) {
            reg_num = std::stoi(argv[i + 1]);
            i += 1;
        } else if (argv[i][0] != '-') {
            if (input_file_path.empty()) {
                input_file_path = argv[i];
            }
        } else if (argv[i] == "-g"sv) {
            loop_sea = true;
            dump_graph = true;
        }
    }
    if (emit_llvm and not silent) {
        print_usage();
        return 1;
    }

    LOG_DEBUG << BOOST_LIB_VERSION;

    if (target_file_path.empty()) {
        auto base_start = input_file_path.rfind('/') + 1;
        if (base_start == std::string::npos)
            base_start = 0;
        auto base_end = input_file_path.rfind('.');
        auto base = input_file_path.substr(base_start, base_end);
        if (silent)
            if (emit_llvm)
                target_file_path = base + ".ll";
            else
                target_file_path = base;  // output both asm and executable + ".s";
        else
            target_file_path = "a.out";
    }
    parse_file(input_file_path);
    // syntax_tree.print();
    LOG_INFO << "generate syntax tree successfully";

    AST ast(&syntax_tree);
    auto astRoot = ast.get_root();
    LOG_INFO << "generate AST successfully";

    // ASTPrinter printer;
    // printer.visit(*astRoot);

    SYSYCBuilder builder;
    builder.visit(*astRoot);

    auto module = builder.getModule();
    module->set_ptr_size((silent and not emit_llvm) ? 4 : sizeof(char *));
    auto ir = module->print();  // 为 Value 编号并打印

    PassManager pass_manager(module);
    if (debug_ir)
        pass_manager.set_debug();

#define add_pass_if(pass, opt) _add_pass_if<pass>(pass_manager, opt)
    config.ir_check = ir_check;
    add_pass_if(RemoveUselessBr, true);
    add_pass_if(Mem2Reg, mem2reg);
    add_pass_if(PolyTest, poly_test);
    add_pass_if(Polyhedral, poly);
    
    add_pass_if(AdHocOptimization, adhoc);
    add_pass_if(DeadGlobalElimination, dead_global);
    add_pass_if(FuncInfo, func_info);
    add_pass_if(LoopMerge, loop_merge);
    add_pass_if(LoopUnrolling, loop_unroll);  // LoopUnroll 前不能死代码消除，否则归纳变量结束值可能被删除
    // add_pass_if(LoopInvMotion, loop_inv);
    add_pass_if(DivisionInvariant, div_inv);
    add_pass_if(TailRecursionElim, tail_opt);
    add_pass_if(LoopInfo, loop_info);
    add_pass_if(AlgebraicSimplification, algebraic_simplification);
    add_pass_if(InstructionSimplify, inst_simpl);
    add_pass_if(DeadCode, dead_code);
    add_pass_if(FuncInline, func_inline);
    add_pass_if(GloVarLocal, glo_var_local);
    add_pass_if(LocalCommonExpression, lce);
    add_pass_if(IVReduction, iv_red);
    // 必须放到 LoopUnrolling 和 LoopInvMotion 之后，后面需要接一个控制流化简处理改写分支产生的空块和异常 phi
    add_pass_if(SCCP, sccp);
    add_pass_if(DeadCode, dead_code);
    add_pass_if(SimplifyCFG, simplify_cfg);
    if (simplify_cfg && loop_sea) {
        pass_manager.pass_list.push_back({std::make_shared<loop_search>(module, dump_graph), false});
    }
    add_pass_if(LocalCommonExpression, lce);
    add_pass_if(Peephole, peephole);
    add_pass_if(LoopInvMotion, loop_inv);
    add_pass_if(AlgebraicSimplification, algebraic_simplification);
    add_pass_if(GepElimination, gep_elim);
    add_pass_if(TransformSelect,transform_select); //transfer
    // add_pass_if(Mem2Reg, mem2reg);
   
    add_pass_if(AlgebraicSimplification, algebraic_simplification && (func_inline || glo_var_local || gep_elim));
    add_pass_if(LoopInvMotion, loop_inv && (func_inline || glo_var_local || gep_elim));
    add_pass_if(MulWeaken, mul_weaken);
    add_pass_if(UselessOperationEli, useless_eli);
    add_pass_if(SCCP, sccp);
    // ADCE 完建议跟一个控制流化简，不然可能需要处理空phi/不可达块/undef 等情况
    // ADCE 会改写控制流（循环之外的也可能改写），因此不建议直接替代 dead code，放置顺序尽量靠后
    // (测试发现会导致循环优化 Pass / 指令化简 Pass 挂掉)
    add_pass_if(ADCE, adce);
    add_pass_if(SimplifyCFG, simplify_cfg);
    add_pass_if(Branch, branch_opt); // 尽量靠后，BranchOpt会引入新的terminator (switch)，很多pass没有处理
    add_pass_if(SimplifyCFG, simplify_cfg);
    add_pass_if(DeadCode, dead_code);
    add_pass_if(LowerIR, lower_ir && silent);
    add_pass_if(DeadCode, lower_ir and dead_code);
    add_pass_if(SimplifyCFG, simplify_cfg);
    
    if (loop_sea)
        pass_manager.pass_list.push_back({std::make_shared<loop_search>(module, dump_graph), false});
#undef add_pass_if

    pass_manager.run();

    if (post_dom) {
        auto postdom = std::make_shared<PostDominators>(module);
        postdom->run();
        postdom->log();
    }
    if (dom) {
        auto dom = std::make_shared<Dominators>(module);
        dom->run();
        dom->log();
    }

    ir = module->print();
    ir = "target triple = \"x86_64-pc-linux-gnu\"\n\n" + ir;
    if (emit_llvm) {
        std::ofstream output_stream(target_file_path);
        output_stream << ir;
        output_stream.close();
    } else if (silent) {
        std::string library_path = argv[0];
        library_path.erase(library_path.rfind('/') + 1);
        auto base = target_file_path.substr(0, target_file_path.rfind('.'));
        auto silent_path = base + ".s";
        auto ll_temp = base + ".ll";
        std::ofstream output_stream(silent_path), ll_output_stream(ll_temp);
        ll_output_stream << ir;
        ll_output_stream.close();
        Codegen cg(builder.getModule(), output_stream, reg_num);
        cg.run();
        if (std::find_if(module->get_functions().begin(), module->get_functions().end(), [](auto f) {
                return f->get_name() == "memset32" and not f->get_use_list().empty();
            }) != module->get_functions().end()) {
            std::ifstream memset_os(library_path + "../lib/memset.s");
            output_stream << memset_os.rdbuf();
        }
        if (std::find_if(module->get_functions().begin(), module->get_functions().end(), [](auto f) {
                return f->get_name() == "memcpy_arm" and not f->get_use_list().empty();
            }) != module->get_functions().end()) {
            std::ifstream memcpy_os(library_path + "../lib/memcpy.s");
            output_stream << memcpy_os.rdbuf();
        }
        output_stream.close();

        // arm-linux-gnueabihf-gcc test.s lib/sylib.c
        // qemu-arm -L /usr/arm-linux-gnueabihf/ ./a.out
        // or in raspberry pi: gcc test.s lib/sylib.c

#ifdef LOCAL_TEST

#ifdef __amd64__
        library_path += "../lib/sylib.c";
        auto command =
            "arm-linux-gnueabihf-gcc -static -g " + silent_path + " " + library_path + " -o " + target_file_path;
#else
        library_path += "lib/libsylib.a";
        auto command = "gcc -march=armv7-a -static " + silent_path + " " + library_path + " -o " + target_file_path;
#endif
        LOG_INFO << "compile command:\n" << command;
        system(command.c_str());

    } else {
        auto pos = input_file_path.rfind('.');
        auto ll = input_file_path.substr(0, pos) + ".ll";
        std::ofstream output_stream(ll);
        output_stream << ir;
        output_stream.close();
        try {
            // library path is relative to cc
            std::string library_path = argv[0];
            library_path.erase(library_path.rfind('/') + 1) += "lib/libsylib.a";
            auto clang_command = "clang -static -o " + target_file_path + " " + ll + " " + library_path;
            auto ret = std::system(clang_command.c_str());
            if (ret != 0)
                return 1;
        } catch (std::exception &e) {
            std::cerr << e.what();
            return 1;
        }
        // 删除临时的 ir 文件
        // 貌似处理中文路径有问题╮(╯▽╰)╭
        // remove(ll.c_str());
#endif
    }
    return 0;
}

#include "Polyhedral.hh"

#include "AST.hpp"
#include "AlgebraicSimplification.hh"
#include "DeadCode.hh"
#include "LocalCommonExpression.hh"
#include "PolyBuilder.hh"
#include "PolyTest.hh"
#include "SyntaxTree.hh"
#include "lexer.hh"
#include "parser.hh"


extern SyntaxTree syntax_tree;
#include "utils.hh"

extern SyntaxTree syntax_tree;

// bool is_valid_loop(Loop *);
using isl::manage;
void Polyhedral::run() {
    AlgebraicSimplification{m_}.run();
    loop_info.run();
    // Dominators dom{m_};
    dom.run();
    // LocalCommonExpression{m_}.run();
    // DeadCode{m_}.run();
    std::vector<Loop *> loop_list;
    std::vector<Loop *> loop_stack;
    for (auto &l : loop_info) {
        if (l.get_parent_loop() == nullptr)
            loop_stack.push_back(&l);
    }

    while (!loop_stack.empty()) {
        auto l = loop_stack.back();
        loop_stack.pop_back();
        loop_list.push_back(l);
        for (auto sub : l->get_sub_loops())
            loop_stack.push_back(sub);
    }

    for (auto l : loop_list) {
        if (scheduled.find(l) != scheduled.end())
            continue;
        auto loop = *l;
        auto loop_end = loop.get_end();
        LOG_DEBUG<<(*(loop_end)).print();
        if (is_valid_loop(l)) {
            if (l->get_sub_loops().empty())
                goto end;
            isl_ctx *ctx_ptr = isl_ctx_alloc();
            isl::ctx ctx(ctx_ptr);
            isl_options_set_on_error(ctx.get(), ISL_ON_ERROR_ABORT);
            std::vector<std::shared_ptr<Instruction>> inst_list;
            std::unordered_map<std::shared_ptr<Instruction>, std::shared_ptr<PolyStmt>> inst2stmt;
            std::unordered_map<Loop *, std::shared_ptr<LoopExeOrder>> loop2order;
            std::unordered_map<std::string, std::shared_ptr<PolyStmt>> name2stmt;
            // isl_space *empty_space = isl_space_params_alloc(ctx, 0);
            // auto empty_set = isl_union_set_empty(empty_space);
            domain = isl::union_set::empty(ctx);
            // auto empty_map = isl_union_map_empty(empty_space);
            isl::union_map schedule = isl::union_map::empty(ctx);
            isl::union_map reads = isl::union_map::empty(ctx);
            isl::union_map writes = isl::union_map::empty(ctx);
            int depth = l->get_sub_depth();
            for (auto bb : loop.get_blocks()) {
                for (auto inst : *bb) {
                    // TODO: 暂时只处理store
                    if (inst->is_call())
                        goto end;
                    if (inst->is_store()) {
                        
                        auto inst_bb = inst->get_parent();
                        //(是否为latch) ||  label_latch 支配 inst的BB
                        if (!(inst_bb == loop_end.get() || dom.is_dominator(loop_end.get(), inst_bb))) {
                            auto inst_loop = loop_info.get_inner_loop(inst_bb);
                            if (inst_loop == l)
                                goto end;

                            // inst_loop的guard支配loop的end
                            auto inst_loop_guard = inst_loop->get_preheader()->get_pre_basic_blocks().back();
                            LOG_DEBUG<<(*(inst_loop_guard)).print();
                            auto inst_loop_parent = inst_loop->get_parent_loop();
                            LOG_DEBUG<<(*(inst_loop_parent->get_end())).print();
                            while (inst_loop_parent) {
                                if (!dom.is_dominator(inst_loop_parent->get_end().get(), inst_loop_guard))
                                    goto end;

                                inst_loop_guard = inst_loop_parent->get_preheader()->get_pre_basic_blocks().back();
                                inst_loop_parent = inst_loop_parent->get_parent_loop();
                            }
                        }
                        inst_list.push_back(inst);
                        // domain
                        
                        auto inner = loop_info.get_inner_loop(inst->get_parent());
                        std::vector<Loop::InductionVar *> bounds;
                        for (auto cl = inner;; cl = cl->get_parent_loop()) {
                            bounds.push_back(cl->get_bound_IV().get());
                            if (cl == l)
                                break;
                        }
                        LOG_DEBUG<<(*inst).print();
                        std::reverse(bounds.begin(), bounds.end());
                        auto stmt_name = get_stmt_name(inst.get());
                        auto stmt = std::shared_ptr<PolyStmt>(new PolyStmt(inst, l, *this, bounds));
                        
                        name2stmt.insert({stmt_name, stmt});
                        inst2stmt.insert({inst, stmt});
                        LOG_DEBUG << stmt->get_domain_str();
                        domain = domain.unite(stmt->get_domain(ctx));
                    }
                }
            }
            // domain ready
            build_loop_order(l, inst_list, loop2order);
            for (auto inst : inst_list) {
                std::vector<int> sch_idxs;
                std::vector<Loop *> loop_trace;
                auto inst_loop = loop_info.get_inner_loop(inst->get_parent());
                sch_idxs.push_back(loop2order.at(inst_loop)->get_idx(inst.get()));
                loop_trace.push_back(inst_loop);
                while (inst_loop != l) {
                    auto par = inst_loop->get_parent_loop();
                    loop_trace.push_back(par);
                    sch_idxs.push_back(loop2order.at(par)->get_idx(inst_loop));
                    inst_loop = par;
                }
                std::reverse(sch_idxs.begin(), sch_idxs.end());
                std::reverse(loop_trace.begin(), loop_trace.end());
                std::string sch_str;
                sch_str += "[";
                sch_str += get_iv_name(loop_trace[0]->get_bound_IV()->inst.get());
                sch_str += ",";
                sch_str += std::to_string(sch_idxs[0]);
                for (int i = 1; i < sch_idxs.size(); i++) {
                    sch_str += ",";
                    sch_str += get_iv_name(loop_trace[i]->get_bound_IV()->inst.get());
                    sch_str += ",";
                    sch_str += std::to_string(sch_idxs[i]);
                }
                for (int i = sch_idxs.size(); i < depth; i++) {
                    sch_str += ",";
                    sch_str += std::to_string(0);
                    sch_str += ",";
                    sch_str += std::to_string(0);
                }
                sch_str += "]";
                sch_str = "{ " + inst2stmt.at(inst)->get_name_idx() + " -> " + sch_str + " }";
                LOG_DEBUG << sch_str;
                auto sch = isl::union_map(ctx, sch_str);
                schedule = schedule.unite(sch);

                // mem
                // read
                auto stmt = inst2stmt.at(inst);
                auto stmt_reads = isl::union_map::empty(ctx);
                for (auto read_inst : stmt->get_reads()) {
                    LOG_DEBUG<<(*read_inst).print();
                    auto mem = PolyMemAccess(read_inst->get_operand(0).get());
                    if (!mem.valid){
                        LOG_DEBUG<<"mem valid"<<(*read_inst).print();
                        continue;
                    }
                    std::string read_str;
                    read_str += stmt->get_name_idx();
                    read_str += " -> ";
                    auto idxs_str = mem.get_str(stmt->idxs, *this);
                    if (idxs_str.empty()){
                        LOG_DEBUG<<"empty():"<<(*read_inst).print();
                        continue;
                    }
                    read_str += idxs_str;
                    read_str = "{ " + read_str + " }";
                    LOG_DEBUG << "reads of " << stmt->get_name() << ": " << read_str;
                    stmt_reads = stmt_reads.unite(isl::union_map(ctx, read_str));
                }
                reads = reads.unite(stmt_reads);
                // write
                auto stmt_writes = isl::union_map::empty(ctx);
                for (auto write_inst : stmt->get_writes()) {
                    auto mem = PolyMemAccess(write_inst->get_operand(1).get());
                    if (!mem.valid){
                        LOG_DEBUG<<"writes wrong";
                        continue;
                    }
                    std::string write_str;
                    write_str += stmt->get_name_idx();
                    write_str += " -> ";
                    auto idxs_str = mem.get_str(stmt->idxs, *this);
                    if (idxs_str.empty())
                        continue;
                    write_str += idxs_str;
                    write_str = "{ " + write_str + " }";
                    LOG_DEBUG << "writes of " << stmt->get_name() << ": " << write_str;
                    stmt_writes = stmt_writes.unite(isl::union_map(ctx, write_str));
                }
                writes = writes.unite(stmt_writes);
            }
            // schedule ready
            isl_printer *p;
            p = isl_printer_to_str(ctx.get());
            p = isl_printer_print_union_set(p, domain.get());
            LOG_DEBUG << "domain: " << isl_printer_get_str(p);
            isl_printer_free(p);

            p = isl_printer_to_str(ctx.get());
            p = isl_printer_print_union_map(p, schedule.get());
            LOG_DEBUG << "schedule: " << isl_printer_get_str(p);
            isl_printer_free(p);

            p = isl_printer_to_str(ctx.get());
            p = isl_printer_print_union_map(p, reads.get());
            LOG_DEBUG << "reads: " << isl_printer_get_str(p);
            isl_printer_free(p);

            p = isl_printer_to_str(ctx.get());
            p = isl_printer_print_union_map(p, writes.get());
            LOG_DEBUG << "writes: " << isl_printer_get_str(p);
            isl_printer_free(p);

            // read&write ready

            // auto empty = isl_union_map_empty(isl_union_map_get_space(schedule.get()));
            auto empty_map = isl::union_map::empty(ctx);
            isl_union_map *dep_raw, *dep_waw, *dep_war;
            // compute RAW dependencies
            isl_union_map_compute_flow(
                reads.copy(), writes.copy(), empty_map.release(), schedule.copy(), &dep_raw, nullptr, nullptr, nullptr);
            // compute WAR, WAW dependencies
            isl_union_map_compute_flow(
                writes.copy(), writes.copy(), reads.copy(), schedule.copy(), &dep_waw, &dep_war, nullptr, nullptr);
            // 调度约束
            isl::schedule_constraints sc;
            // 所有的约束，即 raw+war+waw
            auto dep = isl::manage(dep_waw).unite(isl::manage(dep_war)).unite(isl::manage(dep_raw));
            isl::union_map validity(dep), coincidence(dep), proximity(dep);
            // 调用了成员函数之后记得赋值给自身
            sc = sc.on_domain(isl::manage(domain.copy()));
            // 设置需要满足的约束（即依赖关系）
            sc = sc.set_validity(validity);
            sc = sc.set_coincidence(coincidence);
            sc = sc.set_proximity(proximity);
            auto sched = sc.compute_schedule();  // 调用isl的调度器进行求解，好像是ppcg里的调度算法
            // 需要注意的是isl的scheduler并不生成分块后的代码，需要对各个band进行手动分块

            p = isl_printer_to_str(ctx.get());
            p = isl_printer_print_schedule(p, sched.get());
            LOG_DEBUG << "schedule: " << isl_printer_get_str(p);
            isl_printer_free(p);

            auto root = sched.get_root();
            root = root.map_descendant_bottom_up(optimize_band_node);  // polly
            sched = root.schedule();

            auto build = isl::ast_build(ctx);
            auto isl_ast = build.node_from(sched);
            // 目前想法是再parse一次生成的c代码，跑一遍builder替换掉原有的scop
            auto ast_string = isl_ast.to_C_str();
            LOG_DEBUG << ast_string;
            // 为了方便重用我们的parser，需要用一个函数wrap一下，把isl生成的ast放进去
            ast_string = "void pseudo_header() {\n" + ast_string + "}\n";

            yy_scan_string(ast_string.c_str());
            yyparse();
            AST ast(&syntax_tree);
            auto astRoot = ast.get_root();
            LOG_DEBUG<<"AST：";
        
            ir_inserter.set_insert_point(l->get_exit());
            ir_inserter.add_loop_exit_block(l->get_exit().get());
            ir_inserter.add_loop_before_block(l->get_preheader()->get_pre_basic_blocks_not_ref().back());
            
            poly_cfg.add_loop_exit_block(l->get_exit().get());
            poly_cfg.add_loop_before_block(l->get_preheader()->get_pre_basic_blocks_not_ref().back());
            ir_inserter.name2stmt = std::move(name2stmt);
            ir_inserter.visit(*astRoot);
            poly_cfg.transform_cfg();
            add_scheduled(l);
            // isl_ctx_free(ctx);
        }
    end:;
    }
}

bool Polyhedral::is_valid_loop(Loop *L) {
    auto &subs = L->get_sub_loops();
    // 暂不考虑多个并列的子循环
    if (subs.size() > 1)
        return false;
    for (auto sub : subs) {
        if (!is_valid_loop(sub))
            return false;
    }
    if (L->get_bound_IV() == nullptr)
        return false;
    // 目前归纳变量必须每次递增1
    if (L->get_bound_IV()->get_const_step_val() != 1)
        return false;
    // 目前必须是常数起始值
    if (!L->get_bound_IV()->get_const_init_val().has_value())
        return false;
    // 目前终止值必须是最外层循环的不变式
    // for (i = 0; i < N; i++)
    //   for (j = 0; j < i; j++)
    // 这样的该怎么处理？
    // if (!loop.is_loop_invariant(L->get_bound_IV()->final_val.get()))
    //     return false;
    // 目前必须是递增的循环
    if (L->get_bound_IV()->direction != Loop::InductionVar::direction_t::increasing)
        return false;

    // 不在子循环中的块要支配循环结尾
    // for (auto bb : L->get_blocks()) {
    //     if (!(bb == L->get_end().get() || dom.is_dominator(L->get_end().get(), bb))) {
    //         auto bb_loop = loop_info.get_inner_loop(bb);
    //         if (bb_loop == L)
    //             return false;
    //     }
    // }
    return true;
}
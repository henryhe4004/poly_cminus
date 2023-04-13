/* Copyright 2016-2017 Tobias Grosser
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Tobias Grosser, Weststrasse 47, CH-8003, Zurich
 */
#include "AST.hpp"
#include "PolyBuilder.hh"
#include "PolyTest.hh"
#include "Polyhedral.hh"
#include "SyntaxTree.hh"
#include "lexer.hh"
#include "parser.hh"

#include <isl/constraint.h>
#include <isl/options.h>
#include <isl/typed_cpp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
using isl::manage;
extern SyntaxTree syntax_tree;

static void die_impl(const char *file, int line, const char *message) {
    fprintf(stderr, "Assertion failed in %s:%d %s\n", file, line, message);
    exit(EXIT_FAILURE);
}

static void assert_impl(bool condition, const char *file, int line, const char *message) {
    if (condition)
        return;

    return die_impl(file, line, message);
}

#define die(msg) die_impl(__FILE__, __LINE__, msg)
#undef assert
#define assert(exp) assert_impl(exp, __FILE__, __LINE__, #exp)

#include "isl_test_cpp-generic.cc"

/* Test that isl_bool values are returned correctly.
 *
 * In particular, check the conversion to bool in case of true and false, and
 * exception throwing in case of error.
 */
static void test_return_bool(isl::ctx ctx) {
    isl::set empty(ctx, "{ : false }");
    isl::set univ(ctx, "{ : }");
    isl::set null;

    bool b_true = empty.is_empty();
    bool b_false = univ.is_empty();
    bool caught = false;
    try {
        null.is_empty();
        die("no exception raised");
    } catch (const isl::exception_invalid &e) {
        caught = true;
    }

    assert(b_true);
    assert(!b_false);
    assert(caught);
}

/* Test that return values are handled correctly.
 *
 * Test that isl C++ objects, integers, boolean values, and strings are
 * returned correctly.
 */
static void test_return(isl::ctx ctx) {
    test_return_obj(ctx);
    test_return_int(ctx);
    test_return_bool(ctx);
    test_return_string(ctx);
}

/* Test that foreach functions are modeled correctly.
 *
 * Verify that lambdas are correctly called as callback of a 'foreach'
 * function and that variables captured by the lambda work correctly. Also
 * check that the foreach function handles exceptions thrown from
 * the lambda and that it propagates the exception.
 */
static void test_foreach(isl::ctx ctx) {
    isl::set s(ctx, "{ [0]; [1]; [2] }");

    std::vector<isl::basic_set> basic_sets;

    auto add_to_vector = [&](isl::basic_set bs) { basic_sets.push_back(bs); };

    s.foreach_basic_set(add_to_vector);

    assert(basic_sets.size() == 3);
    assert(isl::set(basic_sets[0]).is_subset(s));
    assert(isl::set(basic_sets[1]).is_subset(s));
    assert(isl::set(basic_sets[2]).is_subset(s));
    assert(!basic_sets[0].is_equal(basic_sets[1]));

    auto fail = [&](isl::basic_set bs) { throw "fail"; };

    bool caught = false;
    try {
        s.foreach_basic_set(fail);
        die("no exception raised");
    } catch (char const *s) {
        caught = true;
    }
    assert(caught);
}

/* Test the functionality of "every" functions.
 *
 * In particular, test the generic functionality and
 * test that exceptions are properly propagated.
 */
static void test_every(isl::ctx ctx) {
    isl::union_set us(ctx, "{ A[i]; B[j] }");

    test_every_generic(ctx);

    auto fail = [](isl::set s) -> bool { throw "fail"; };
    bool caught = false;
    try {
        us.every_set(fail);
        die("no exception raised");
    } catch (char const *s) {
        caught = true;
    }
    assert(caught);
}

/* Test that an exception is generated for an isl error and
 * that the error message is captured by the exception.
 * Also check that the exception can be copied and that copying
 * does not throw any exceptions.
 */
static void test_exception(isl::ctx ctx) {
    isl::multi_union_pw_aff mupa(ctx, "[]");
    isl::exception copy;

    static_assert(std::is_nothrow_copy_constructible<isl::exception>::value,
                  "exceptions must be nothrow-copy-constructible");
    static_assert(std::is_nothrow_assignable<isl::exception, isl::exception>::value,
                  "exceptions must be nothrow-assignable");

    try {
        auto umap = isl::union_map::from(mupa);
    } catch (const isl::exception_unsupported &error) {
        die("caught wrong exception");
    } catch (const isl::exception &error) {
        assert(strstr(error.what(), "without explicit domain"));
        copy = error;
    }
    assert(strstr(copy.what(), "without explicit domain"));
}

/* Test basic schedule tree functionality.
 *
 * In particular, create a simple schedule tree and
 * - perform some generic tests
 * - test map_descendant_bottom_up in the failing case
 * - test foreach_descendant_top_down
 * - test every_descendant
 */
static void test_schedule_tree(isl::ctx ctx) {
    auto root = test_schedule_tree_generic(ctx);

    auto fail_map = [](isl::schedule_node node) {
        throw "fail";
        return node;
    };
    auto caught = false;
    try {
        root.map_descendant_bottom_up(fail_map);
        die("no exception raised");
    } catch (char const *s) {
        caught = true;
    }
    assert(caught);

    int count = 0;
    auto inc_count = [&count](isl::schedule_node node) {
        count++;
        return true;
    };
    root.foreach_descendant_top_down(inc_count);
    assert(count == 8);

    count = 0;
    auto inc_count_once = [&count](isl::schedule_node node) {
        count++;
        return false;
    };
    root.foreach_descendant_top_down(inc_count_once);
    assert(count == 1);

    auto is_not_domain = [](isl::schedule_node node) { return !node.isa<isl::schedule_node_domain>(); };
    assert(root.child(0).every_descendant(is_not_domain));
    assert(!root.every_descendant(is_not_domain));

    auto fail = [](isl::schedule_node node) {
        throw "fail";
        return true;
    };
    caught = false;
    try {
        root.every_descendant(fail);
        die("no exception raised");
    } catch (char const *s) {
        caught = true;
    }
    assert(caught);

    auto domain = root.as<isl::schedule_node_domain>().domain();
    auto filters = isl::union_set(ctx, "{}");
    auto collect_filters = [&filters](isl::schedule_node node) {
        if (node.isa<isl::schedule_node_filter>()) {
            auto filter = node.as<isl::schedule_node_filter>();
            filters = filters.unite(filter.filter());
        }
        return true;
    };
    root.every_descendant(collect_filters);
    assert(domain.is_equal(filters));
}

/* Test basic AST generation from a schedule tree.
 *
 * In particular, create a simple schedule tree and
 * - perform some generic tests
 * - test at_each_domain in the failing case
 */
static void test_ast_build(isl::ctx ctx) {
    auto schedule = test_ast_build_generic(ctx);

    bool do_fail = true;
    int count_ast_fail = 0;
    auto fail_inc_count_ast = [&count_ast_fail, &do_fail](isl::ast_node node, isl::ast_build build) {
        count_ast_fail++;
        if (do_fail)
            throw "fail";
        return node;
    };
    auto build = isl::ast_build(ctx);
    build = build.set_at_each_domain(fail_inc_count_ast);
    auto caught = false;
    try {
        auto ast = build.node_from(schedule);
    } catch (char const *s) {
        caught = true;
    }
    assert(caught);
    assert(count_ast_fail > 0);
    auto build_copy = build;
    int count_ast = 0;
    auto inc_count_ast = [&count_ast](isl::ast_node node, isl::ast_build build) {
        count_ast++;
        return node;
    };
    build_copy = build_copy.set_at_each_domain(inc_count_ast);
    auto ast = build_copy.node_from(schedule);
    assert(count_ast == 2);
    count_ast_fail = 0;
    do_fail = false;
    ast = build.node_from(schedule);
    assert(count_ast_fail == 2);
}

/* Basic test of the templated interface.
 *
 * Intersecting the domain of an access relation
 * with statement instances should be allowed,
 * while intersecting the range with statement instances
 * should result in a compile-time error.
 */
static void test_typed(isl::ctx ctx) {
    struct ST {};
    struct AR {};
    isl::typed::map<ST, AR> access(ctx, "{ S[i, j] -> A[i] }");
    isl::typed::set<ST> instances(ctx, "{ S[i, j] : 0 <= i, j < 10 }");

#ifndef COMPILE_ERROR
    access.intersect_domain(instances);
#else
    access.intersect_range(instances);
#endif
}

static isl_stat check_injective(__isl_take isl_map *map, void *user) {
    int *injective = reinterpret_cast<int *>(user);

    *injective = isl_map_is_injective(map);
    isl_map_free(map);

    if (*injective < 0 || !*injective)
        return isl_stat_error;

    return isl_stat_ok;
}

// copied from isl_test.c 可以研究一下
int test_one_schedule(isl_ctx *ctx,
                      const char *d,
                      const char *w,
                      const char *r,
                      const char *s,
                      int tilable,
                      int parallel,
                      Module *m_) {
    int i;
    isl_union_set *D;
    isl_union_map *W, *R, *S;
    isl_union_map *empty;
    isl_union_map *dep_raw, *dep_war, *dep_waw, *dep;
    isl_union_map *validity, *proximity, *coincidence;
    isl_union_map *schedule;
    isl_union_map *test;
    isl_union_set *delta;
    isl_union_set *domain;
    isl_set *delta_set;
    isl_set *slice;
    isl_set *origin;
    isl_schedule_constraints *sc;
    isl_schedule *sched;
    int is_nonneg, is_parallel, is_tilable, is_injection, is_complete;
    isl_size n;

    D = isl_union_set_read_from_str(ctx, d);
    W = isl_union_map_read_from_str(ctx, w);
    R = isl_union_map_read_from_str(ctx, r);
    S = isl_union_map_read_from_str(ctx, s);

    W = isl_union_map_intersect_domain(W, isl_union_set_copy(D));
    R = isl_union_map_intersect_domain(R, isl_union_set_copy(D));

    empty = isl_union_map_empty(isl_union_map_get_space(S));
    isl_union_map_compute_flow(
        isl_union_map_copy(R), isl_union_map_copy(W), empty, isl_union_map_copy(S), &dep_raw, NULL, NULL, NULL);
    isl_union_map_compute_flow(isl_union_map_copy(W),
                               isl_union_map_copy(W),
                               isl_union_map_copy(R),
                               isl_union_map_copy(S),
                               &dep_waw,
                               &dep_war,
                               NULL,
                               NULL);

    dep = isl_union_map_union(dep_waw, dep_war);
    dep = isl_union_map_union(dep, dep_raw);
    validity = isl_union_map_copy(dep);
    coincidence = isl_union_map_copy(dep);
    proximity = isl_union_map_copy(dep);

    sc = isl_schedule_constraints_on_domain(isl_union_set_copy(D));
    sc = isl_schedule_constraints_set_validity(sc, validity);
    sc = isl_schedule_constraints_set_coincidence(sc, coincidence);
    sc = isl_schedule_constraints_set_proximity(sc, proximity);
    sched = isl_schedule_constraints_compute_schedule(sc);
    auto jjkkk = isl::manage_copy(sched);

    auto root = jjkkk.get_root();
    root = root.map_descendant_bottom_up(optimize_band_node);  // polly
    jjkkk = root.schedule();

    // debug
    FILE *out = fopen("sched.yaml", "w");
    isl_printer *p = isl_printer_to_file(ctx, out);
    p = isl_printer_print_schedule(p, jjkkk.get());
    isl_printer_free(p);
    fclose(out);

    auto build = isl::ast_build(ctx);
    auto islast = build.node_from(jjkkk);
    // 目前想法是再parse一次生成的c代码，跑一遍builder替换掉原有的scop
    auto ast_string = islast.to_C_str();
    // 为了方便重用我们的parser，需要用一个函数包一下，把isl生成的ast放进去
    ast_string = "void pseudo_header() {\n" + ast_string + "}\n";
    std::cout << ast_string << std::endl;
    // 然后还需要加for语句和 += 这样的赋值，并且min和max可以换成select，别的应该就差不多了
    yy_scan_string(ast_string.c_str());
    yyparse();
    AST ast(&syntax_tree);
    auto astRoot = ast.get_root();
    LOG(INFO) << "AST";

    ASTPrinter printer;
    printer.visit(*astRoot);
    PolyBuilder ir_inserter(m_, nullptr);
    ir_inserter.visit(*astRoot);

    // debug end

    schedule = isl_schedule_get_map(sched);
    isl_schedule_free(sched);
    isl_union_map_free(W);
    isl_union_map_free(R);
    isl_union_map_free(S);

    is_injection = 1;
    isl_union_map_foreach_map(schedule, &check_injective, &is_injection);

    domain = isl_union_map_domain(isl_union_map_copy(schedule));
    is_complete = isl_union_set_is_subset(D, domain);
    isl_union_set_free(D);
    isl_union_set_free(domain);

    test = isl_union_map_reverse(isl_union_map_copy(schedule));
    test = isl_union_map_apply_range(test, dep);
    test = isl_union_map_apply_range(test, schedule);

    delta = isl_union_map_deltas(test);
    n = isl_union_set_n_set(delta);
    if (n < 0) {
        isl_union_set_free(delta);
        return -1;
    }
    if (n == 0) {
        is_tilable = 1;
        is_parallel = 1;
        is_nonneg = 1;
        isl_union_set_free(delta);
    } else {
        isl_size dim;

        delta_set = isl_set_from_union_set(delta);

        slice = isl_set_universe(isl_set_get_space(delta_set));
        for (i = 0; i < tilable; ++i)
            slice = isl_set_lower_bound_si(slice, isl_dim_set, i, 0);
        is_tilable = isl_set_is_subset(delta_set, slice);
        isl_set_free(slice);

        slice = isl_set_universe(isl_set_get_space(delta_set));
        for (i = 0; i < parallel; ++i)
            slice = isl_set_fix_si(slice, isl_dim_set, i, 0);
        is_parallel = isl_set_is_subset(delta_set, slice);
        isl_set_free(slice);

        origin = isl_set_universe(isl_set_get_space(delta_set));
        dim = isl_set_dim(origin, isl_dim_set);
        if (dim < 0)
            origin = isl_set_free(origin);
        for (i = 0; i < dim; ++i)
            origin = isl_set_fix_si(origin, isl_dim_set, i, 0);

        delta_set = isl_set_union(delta_set, isl_set_copy(origin));
        delta_set = isl_set_lexmin(delta_set);

        is_nonneg = isl_set_is_equal(delta_set, origin);

        isl_set_free(origin);
        isl_set_free(delta_set);
    }

    if (is_nonneg < 0 || is_parallel < 0 || is_tilable < 0 || is_injection < 0 || is_complete < 0)
        return -1;
    if (!is_complete)
        isl_die(ctx, isl_error_unknown, "generated schedule incomplete", return -1);
    if (!is_injection)
        isl_die(ctx, isl_error_unknown, "generated schedule not injective on each statement", return -1);
    if (!is_nonneg)
        isl_die(ctx, isl_error_unknown, "negative dependences in generated schedule", return -1);
    if (!is_tilable)
        isl_die(ctx, isl_error_unknown, "generated schedule not as tilable as expected", return -1);
    if (!is_parallel)
        isl_die(ctx, isl_error_unknown, "generated schedule not as parallel as expected", return -1);

    return 0;
}

isl::schedule_node tile_node(isl::schedule_node node, int default_tile_size) {
    // we've run checks before, just do the cast
    auto band = node.as<isl::schedule_node_band>();
    //返回band的局部schedule的space   ps:无论何时从头开始创建新的集合、关系或类似对象，都需要使用isl空间指定其所在的空间
    auto space = manage(isl_schedule_node_band_get_space(node.get()));
    //多重表达式表示零个或多个基本表达式的序列，所有多重表达式都定义在同一个domain上
    //可以使用以下函数为每个输出（或集合）维度创建值为零的多重表达式。
    auto sizes = isl::multi_val::zero(space);
    //重新定义space
    for (unsigned i = 0; i < sizes.size(); ++i) {
        sizes = sizes.set_at(i, default_tile_size);
    }

    auto TileLoopMarkerStr = "Tiles";
    // 标识符用于标识单个维度和维度元组。

     //它们由一个可选名称和一个可选用户指针组成。但是，名称和用户指针不能同时为NULL。具有相同名称但不同指针值的标识符被认为是不同的。类似地，具有不同名称但具有相同指针值的标识符也被认为是不同的。使用相同的对象来表示相等的标识符。因此，可以使用==运算符来测试标识符对的相等性。可以使用以下功能构建、复制、释放、检查和打印标识符。 
     //根据id分配ctx
    auto TileLoopMarker = manage(isl_id_alloc(node.ctx().get(), TileLoopMarkerStr, nullptr));
    // 添加mark节点 mark节点是在原来节点与父节点之间加入
    node = node.insert_mark(TileLoopMarker);
    //
    node = node.child(0);

    node = node.as<isl::schedule_node_band>().tile(sizes);
    //将该节点移动到0节点 0节点应该就是自身？
    node = node.child(0);
    auto PointLoopMarkerStr = "Points";
    auto PointLoopMarker = manage(isl_id_alloc(node.ctx().get(), PointLoopMarkerStr, nullptr));
    node = node.insert_mark(PointLoopMarker);

    // return node.child(0);
    return node.parent();
    // original
    // return band.tile(sizes);
}

isl::set addExtentConstraints(isl::set Set, int VectorWidth) {
    //   unsigned Dims = unsignedFromIslSize(Set.tuple_dim());
    auto space = Set.space();
    auto Dims = isl::multi_val::zero(space).size();
    assert(Dims >= 1);
    isl::space Space = Set.get_space();
    auto LocalSpace = isl_local_space_from_space(Space.release());
    // isl::local_space LocalSpace = isl::local_space(Space);
    auto ExtConstr = isl_constraint_alloc_inequality(
        isl_local_space_copy(LocalSpace));  // isl::constraint::alloc_inequality(LocalSpace);
    ExtConstr = isl_constraint_set_constant_si(ExtConstr, 0);
    ExtConstr = isl_constraint_set_coefficient_si(ExtConstr, isl_dim_type::isl_dim_set, Dims - 1, 1);
    Set = manage(isl_set_add_constraint(Set.release(),
                                        ExtConstr));  // release expconstr  // Set.add_constraint(manage(ExtConstr));
    ExtConstr = isl_constraint_alloc_inequality(LocalSpace);

    ExtConstr = isl_constraint_set_constant_si(ExtConstr, VectorWidth - 1);
    ExtConstr = isl_constraint_set_coefficient_si(ExtConstr, isl_dim_type::isl_dim_set, Dims - 1, -1);
    return manage(isl_set_add_constraint(Set.release(), ExtConstr));
}

isl::set getPartialTilePrefixes(isl::set ScheduleRange, int VectorWidth) {
    auto space = ScheduleRange.space();
    auto sizes = isl::multi_val::zero(space);
    unsigned Dims = sizes.size();  // unsignedFromIslSize(ScheduleRange.tuple_dim());
    assert(Dims >= 1);

    isl::set LoopPrefixes =
        manage(isl_set_drop_constraints_involving_dims(ScheduleRange.copy(), isl_dim_type::isl_dim_set, Dims - 1, 1));
    auto ExtentPrefixes = addExtentConstraints(LoopPrefixes, VectorWidth);
    isl::set BadPrefixes = ExtentPrefixes.subtract(ScheduleRange);
    BadPrefixes = manage(isl_set_project_out(BadPrefixes.release(), isl_dim_type::isl_dim_set, Dims - 1, 1));
    // BadPrefixes.project_out(isl::dim::set, Dims - 1, 1);
    // LoopPrefixes = LoopPrefixes.project_out(isl::dim::set, Dims - 1, 1);
    LoopPrefixes = manage(isl_set_project_out(LoopPrefixes.release(), isl_dim_set, Dims - 1, 1));
    return LoopPrefixes.subtract(BadPrefixes);
}

isl::union_set getDimOptions(isl::ctx Ctx, const char *Option) {
    //   isl::space Space(Ctx, 0, 1);
    auto Space = manage(isl_space_set_alloc(Ctx.get(), 0, 1));
    auto DimOption = isl::set::universe(Space);
    //   auto Id = isl::id::alloc(Ctx, Option, nullptr);
    auto Id = manage(isl_id_alloc(Ctx.get(), Option, nullptr));
    DimOption = manage(isl_set_set_tuple_id(DimOption.release(), Id.release()));
    return isl::union_set(DimOption);
}

isl::union_set getIsolateOptions(isl::set IsolateDomain, unsigned OutDimsNum) {
    // unsigned Dims = unsignedFromIslSize(IsolateDomain.tuple_dim());
    auto space = IsolateDomain.space();
    auto Dims = isl::multi_val::zero(space).size();

    isl::map IsolateRelation = manage(isl_map_from_domain(IsolateDomain.release()));  // isl::map(IsolateDomain);
    // IsolateRelation = IsolateRelation.move_dims(isl::dim::out, 0, isl::dim::in, Dims - OutDimsNum, OutDimsNum);
    IsolateRelation =
        manage(isl_map_move_dims(IsolateRelation.release(), isl_dim_out, 0, isl_dim_in, Dims - OutDimsNum, OutDimsNum));
    isl::set IsolateOption = IsolateRelation.wrap();
    // isl::id Id = isl::id::alloc(IsolateOption.ctx(), "isolate", nullptr);
    auto Id = manage(isl_id_alloc(IsolateOption.ctx().get(), "isolate", nullptr));
    // IsolateOption = IsolateOption.set_tuple_id(Id);
    IsolateOption = manage(isl_set_set_tuple_id(IsolateOption.release(), Id.release()));
    return isl::union_set(IsolateOption);
}

isl::schedule_node isolateFullPartialTiles(isl::schedule_node node, int vector_width) {
    assert(isl_schedule_node_get_type(node.get()) == isl_schedule_node_band);
    node = node.child(0).child(0);

    isl::union_map SchedRelUMap = manage(isl_schedule_node_get_prefix_schedule_relation(node.copy()));
    isl::union_set ScheduleRangeUSet = SchedRelUMap.range();
    isl::set ScheduleRange = ScheduleRangeUSet.as_set();
    // totally don't understand
    isl::set IsolateDomain = getPartialTilePrefixes(ScheduleRange, vector_width);
    auto AtomicOption = getDimOptions(IsolateDomain.ctx(), "atomic");
    isl::union_set IsolateOption = getIsolateOptions(IsolateDomain, 1);
    node = node.parent().parent();
    isl::union_set Options = IsolateOption.unite(AtomicOption);
    isl::schedule_node_band Result = node.as<isl::schedule_node_band>().set_ast_build_options(Options);
    return Result;
}

isl::schedule_node prevect_band(isl::schedule_node node, unsigned dim_to_vectorize, int vector_width) {
    LOG_DEBUG << "dim to vectorize: " << dim_to_vectorize;
    auto space = manage(isl_schedule_node_band_get_space(node.get()));
    auto sizes = isl::multi_val::zero(space);
    LOG_DEBUG << "space: " << sizes.size();
    if (dim_to_vectorize > 0) {
        node = node.as<isl::schedule_node_band>().split(dim_to_vectorize);
        node = node.child(0);
    }
    if (dim_to_vectorize < sizes.size() - 1)
        // not inner most dimension
        node = node.as<isl::schedule_node_band>().split(1);
    isl_printer *p;

    p = isl_printer_to_str(node.ctx().get());
    p = isl_printer_print_schedule_node(p, node.get());
    LOG_DEBUG << "before tiling (prevect): " << isl_printer_get_str(p);
    isl_printer_free(p);

    space = manage(isl_schedule_node_band_get_space(node.get()));
    sizes = isl::multi_val::zero(space);
    LOG_DEBUG << "space: " << sizes.size();
    sizes = sizes.set_at(0, vector_width);  // remember to assign to itself!!!!
    node = node.as<isl::schedule_node_band>().tile(sizes);

    p = isl_printer_to_str(node.ctx().get());
    p = isl_printer_print_schedule_node(p, node.get());
    LOG_DEBUG << "after tiling (prevect): " << isl_printer_get_str(p);
    isl_printer_free(p);

    // processing tiled node
    node = isolateFullPartialTiles(node, vector_width);

    p = isl_printer_to_str(node.ctx().get());
    p = isl_printer_print_schedule_node(p, node.get());
    LOG_DEBUG << "after isolate: " << isl_printer_get_str(p);
    isl_printer_free(p);

    node = node.child(0);
    node = node.as<isl::schedule_node_band>().set_ast_build_options(isl::union_set(node.ctx(), "{ unroll[x] }"));
    // node = node.as<isl::schedule_node_band>().member_set_ast_loop_unroll(0);
    node = isl::manage(isl_schedule_node_band_sink(node.release()));

    p = isl_printer_to_str(node.ctx().get());
    p = isl_printer_print_schedule_node(p, node.get());
    LOG_DEBUG << "after sink: " << isl_printer_get_str(p);
    isl_printer_free(p);

    return node.parent();
}

isl::schedule_node prevect(isl::schedule_node n) {
    auto space = manage(isl_schedule_node_band_get_space(n.get()));
    auto sizes = isl::multi_val::zero(space);
    LOG_DEBUG<<"sizes: "<<sizes.size();
    for (int i = sizes.size() - 1; i >= 0; i--) {
        LOG_DEBUG << "index: " << i;
        if (n.as<isl::schedule_node_band>().member_get_coincident(i)) {
            n = prevect_band(n, i, 4);  // vector width
            break;
        }
    }
    return n;
}

// polly 模仿赛
isl::schedule_node optimize_band_node(isl::schedule_node n) {
    // 只 tile L1缓存
    std::cout << "check band\n";
    // 1. check this is a band node
    if (not n.isa<isl::schedule_node_band>())
        return n;
    if (n.n_children() != 1)
        return n;
    auto band = n.as<isl::schedule_node_band>();
    if (not band.permutable())
        return n;
    auto space = manage(isl_schedule_node_band_get_space(n.get()));
    auto sizes = isl::multi_val::zero(space);
    if (sizes.size() <= 1)
        return n;
    isl_printer *p;
    p = isl_printer_to_str(n.ctx().get());
    p = isl_printer_print_schedule_node(p, n.get());
    LOG_DEBUG << "before tiling: " << isl_printer_get_str(p);
    isl_printer_free(p);

    std::cout << "run optimize_band_node\n";
    n = tile_node(n, 64);

    p = isl_printer_to_str(n.ctx().get());
    p = isl_printer_print_schedule_node(p, n.get());
    LOG_DEBUG << "after tilenode " << isl_printer_get_str(p);
    isl_printer_free(p);

    n = prevect(n);

    p = isl_printer_to_str(n.ctx().get());
    p = isl_printer_print_schedule_node(p, n.get());
    LOG_DEBUG << "after prevect " << isl_printer_get_str(p);
    isl_printer_free(p);

    return n;
}

// 构造 test/poly/sl.c 的调度树，打印至test.yaml
// 可以参考 third_party/isl-isl-0.24/isl_test.c
void test_sl123(isl::ctx ctx) {
    // 先normalize一下
    isl::union_set s1(ctx, "[N] -> { S1[i,j] : 0 <= i < N-2 and 1 <= j < N-2 } ");
    isl::union_set s2(ctx, "[N] -> { S2[i,j,k] : 0 <= i < N-2 and 1 <= j < N-2 and 1 <= k < N-2 }");
    // iteration domain，基本上是找到所在循环的归纳变量就可以了
    auto domain = s1.unite(s2);

    // 修改构造调度树的方式，现在只需传入迭代域，调度和内存访问（读、写）就可以进行依赖分析并生成调度树
    // s1和s2的执行顺序，也可以这样写 isl::union_map(ctx, "S1...").unite(isl::union_map(ctx, "S2..."))
    auto schedule = isl::union_map(ctx, "{ S1[i,j] -> [0,i,0,j,0,0,0]; S2[i,j,k] -> [0,i,0,j,1,k,0] }");
    // 各个instance的load，TODO: 研究一下能否用一维数组表示
    auto reads = isl::union_map(
        ctx,
        "{ S1[i,j] -> a[i+1,j+1,0]; "
        "S2[i,j,k] -> a[i,j+1,k+1]; S2[i,j,k] -> a[i+2,j,k]; S2[i,j,k] -> a[i,j,k]; S2[i,j,k] -> a[i,j+1,k]; "
        "S2[i,j,k] -> a[i,j,k-1]; S2[i,j,k] -> a[i,j,k+1] }");
    // store
    auto writes = isl::union_map(ctx, "{ S1[i,j] -> a[i,j,0]; S2[i,j,k] -> a[i,j,k] }");
    auto empty = isl_union_map_empty(isl_union_map_get_space(schedule.get()));
    isl_union_map *dep_raw, *dep_waw, *dep_war;
    // compute RAW dependencies
    isl_union_map_compute_flow(
        reads.copy(), writes.copy(), empty, schedule.copy(), &dep_raw, nullptr, nullptr, nullptr);
    // compute WAR, WAW dependencies
    isl_union_map_compute_flow(
        writes.copy(), writes.copy(), reads.copy(), schedule.copy(), &dep_waw, &dep_war, nullptr, nullptr);
    // 调度约束
    isl::schedule_constraints sc;
    // 所有的约束，即 raw+war+waw
    auto dep = manage(dep_waw).unite(manage(dep_war)).unite(manage(dep_raw));
    isl::union_map validity(dep), coincidence(dep), proximity(dep);
    // 调用了成员函数之后记得赋值给自身
    sc = sc.on_domain(manage(domain.copy()));
    // 设置需要满足的约束（即依赖关系）
    sc = sc.set_validity(validity);
    sc = sc.set_coincidence(coincidence);
    sc = sc.set_proximity(proximity);
    auto sched = sc.compute_schedule();  // 调用isl的调度器进行求解，好像是ppcg里的调度算法
    // 需要注意的是isl的scheduler并不生成分块后的代码，需要对各个band进行手动分块
    FILE *out = fopen("test.yaml", "w");
    isl_printer *p = isl_printer_to_file(ctx.get(), out);
    p = isl_printer_print_schedule(p, sched.get());
    isl_printer_free(p);
    // tiling TODO
    auto root = sched.get_root();
    root = root.map_descendant_bottom_up(optimize_band_node);  // polly
    sched = root.schedule();

    auto build = isl::ast_build(ctx);
    auto ast = build.node_from(sched);

    std::cout << ast.to_C_str() << std::endl;
    // 这个list也只是一行一行打印而已
    // auto list = ast.to_list();
    // list.foreach([&](isl::ast_node n) {
    //     std::cout << n.to_C_str();
    // });
}
/* Test the (unchecked) isl C++ interface
 *
 * This includes:
 *  - The isl C <-> C++ pointer interface
 *  - Object construction
 *  - Different parameter types
 *  - Different return types
 *  - Foreach functions
 *  - Exceptions
 *  - Spaces
 *  - Schedule trees
 *  - AST generation
 *  - AST expression generation
 *  - Templated interface
 */
void PolyTest::run() {
    isl_ctx *ctx = isl_ctx_alloc();

    isl_options_set_on_error(ctx, ISL_ON_ERROR_CONTINUE);

    // test_pointer(ctx);
    // test_constructors(ctx);
    // test_parameters(ctx);
    // test_return(ctx);
    // test_foreach(ctx);
    // test_every(ctx);
    // test_exception(ctx);
    // test_space(ctx);
    // test_schedule_tree(ctx);
    // test_ast_build(ctx);
    // test_ast_build_expr(ctx);
    // test_typed(ctx);
    // test_sl123(ctx);
    /* Jacobi */
    const char *D, *W, *R, *S;
    D = "[N] -> { S_0[k, i, j] : i >= 0 and i <= -1 + N and j >= 0 and "
        "j <= -1 + N and k >= 0 and k <= -1 + N }";
    W = "{ S_0[k, i, j] -> c[i, j] }";
    R = "{ S_0[k, i, j] -> c[i, j]; "
        "S_0[k, i, j] -> a[i, k]; "
        "S_0[k, i, j] -> b[k, j] }";
    S = "{ S_0[k, i, j] -> [0, k, 0, i, 0, j, 0] }";
    // D = "[N] -> { S[i] : 0 <= i < N }";
    // W = "[N] -> { S[i] -> dst[i] }";
    // R = "[N] -> { S[i] -> src[i] }";
    // S = "[N] -> { S[i] -> [i] }";
    // D = "[N] -> { S1[i,j,k] : 1 <= i < N-1 and 1 <= j < N-1 and 1 <= k < N-1}";
    // W = "{ S1[i,j,k] -> a[i,j,k] }";
    // R = "{ S1[i,j,k] -> a[i-1,j,k]; S1[i,j,k] -> a[i+1,j,k]; S1[i,j,k] -> a[i,j-1,k]; S1[i,j,k] -> a[i,j+1,k]; "
    //     "S1[i,j,k] -> a[i,j,k-1]; S1[i,j,k] -> a[i,j,k+1] }";
    // S = "{ S1[i,j,k] -> [i,j,k] }";
    if (test_one_schedule(ctx, D, W, R, S, 2, 0, m_) < 0)
        LOG_DEBUG << "failed";

    isl_ctx_free(ctx);

    // return EXIT_SUCCESS;
}

// S1: a[i,j] = a[i-1,j] + a[i-1,j-1] + a[i-1,j+1]
// s1': a[i, i+j] = a[i-1,i+j] +a[i-1,i+j-1] + a[i-1,i+j+1]

#include "PolyBuilder.hh"

#include "Polyhedral.hh"
#include "Value.h"
#include "errorcode.hh"
#include "logging.hpp"

#include <functional>
#include <stack>

using std::literals::string_literals::operator""s;

#define CONST_FP(num) ConstantFP::get(static_cast<float>(num), module)
#define CONST_INT(num) ConstantInt::get(num, module)

/// \brief 区分需要获取值还是地址
static bool lval_as_rval{true};
static bool branch;

static int debug_flag = 0;

/// \brief latest visited value
static std::shared_ptr<Value> val;
static std::shared_ptr<Value> ret_val;
static std::shared_ptr<BasicBlock> continue_bb, break_bb, true_block, false_block, ret_bb;
/// \brief basic block counter to avoid name conflicts
static size_t counter{0};
static Function *cur_func;
// Type *cur_arr_inner_type;
static Type *cur_arr_inner_type;
static std::vector<int> cur_arr_shape;
static std::vector<int> glo_init_finished;
static std::shared_ptr<Value> cur_init_base;
static int cur_dim;
static std::shared_ptr<Constant> global_initializer;
static std::vector<std::vector<std::shared_ptr<Constant>>> glo_init;
static int block_depth = 0;
static ASTInitList *empty_init_list;
static ASTConstInitList *empty_const_init_list;

static Type *VOID_T;
static Type *INT1_T;
static Type *INT32_T;
static Type *INT32PTR_T;
static Type *FLOAT_T;
static Type *FLOATPTR_T;

#define _IRBUILDER_ERROR_(str)                                                                                         \
    {                                                                                                                  \
        std::cerr << "Error in IRbuilder-> " << str << std::endl;                                                      \
        std::abort();                                                                                                  \
    }

void PolyBuilder::set_insert_point(std::shared_ptr<BasicBlock> bb) { builder->set_insert_point(bb); }

inline std::shared_ptr<Value> type_cast(std::shared_ptr<Value> _v, Type *_ty, IRBuilder &_builder, Module *_m) {
    std::shared_ptr<Value> casted = _v;
    if (_v->get_type() != _ty) {
        auto constant = dynamic_cast<Constant *>(_v.get());
        if (constant) {
            auto con_int = dynamic_cast<ConstantInt *>(constant);
            if (con_int && _ty == FLOAT_T)
                casted = ConstantFP::get((float)con_int->get_value(), _m);
            else if (!con_int && _ty == INT32_T)
                casted = ConstantInt::get((int)static_cast<ConstantFP *>(constant)->get_value(), _m);
        } else {
            if (_v->get_type() == INT32_T && _ty == FLOAT_T)
                casted = _builder.create_sitofp(_v, FLOAT_T);
            else if (_v->get_type() == FLOAT_T && _ty == INT32_T)
                casted = _builder.create_fptosi(_v, INT32_T);
        }
    }
    return casted;
}

// in our poly IR builder, FuncDef is a pseudo function that is only added for convenience
void PolyBuilder::visit(ASTFuncDef &node) {
    // FunctionType *fun_type;
    // Type *ret_type;
    // std::vector<Type *> param_types;
    // switch (node.type) {
    //     case TYPE_INT:
    //         ret_type = INT32_T;
    //         break;
    //     case TYPE_FLOAT:
    //         ret_type = FLOAT_T;
    //         break;
    //     case TYPE_VOID:
    //         ret_type = VOID_T;
    //         break;
    //     default:
    //         break;
    // }
    // for (auto param : node.param_list) {
    //     param->accept(*this);
    //     param_types.push_back(cur_arr_inner_type);
    // }
    // fun_type = FunctionType::get(ret_type, param_types);
    // auto fun = Function::create(fun_type, node.id, module);
    // scope.push(node.id, fun);
    // scope.enter();
    // cur_func = fun.get();
    // auto entry_bb = BasicBlock::create(module, "entry", fun.get());
    auto main_func = *std::find_if(
        module->get_functions().begin(), module->get_functions().end(), [](auto f) { return f->get_name() == "main"; });
    cur_func = main_func.get();
    counter = main_func->get_num_basic_blocks();

    auto label_before_loop = this->get_loop_before_blocks(); //获取第一个loop_before_loop的前继
    auto label_before_loop_pre_list = label_before_loop->get_pre_basic_blocks_not_ref();//获取其父节点
    auto label_before_loop_pre = label_before_loop_pre_list.back();
    auto label_exit_loop = this->get_loop_exit_blocks();//获取loop_exit
    auto label_exit_loop_succ = label_exit_loop->get_succ_basic_blocks().back();//获取loop_after
    // 根据label_exit_loop删除除开label_before_loop_pre的所有前驱块
    // std::stack<BasicBlock *> delete_BB_stack;
    // std::stack<BasicBlock *> delete_BB_stack_new;
    // std::set<BasicBlock *> visit_BB;
    // delete_BB_stack.push(label_exit_loop);
    // while(!delete_BB_stack.empty()){
    //     BasicBlock* now_BB = delete_BB_stack.top();
    //     delete_BB_stack.pop();
    //     visit_BB.insert(now_BB);
    //     auto now_BB_pre_list = now_BB->get_pre_basic_blocks_not_ref();
    //     for(auto now_BB_pre:now_BB_pre_list){
    //         if(now_BB_pre==label_before_loop_pre||visit_BB.find(now_BB_pre)!=visit_BB.end()){
    //             continue;
    //         }
    //         delete_BB_stack.push(now_BB_pre);
    //     }
    //     LOG_DEBUG<<now_BB->print();
    //     cur_func->remove_not_bb(std::dynamic_pointer_cast<BasicBlock>(now_BB->shared_from_this()));
    //     // delete_BB_stack_new.push(now_BB);
    //     // cur_func->remove_unreachable_basic_block(std::dynamic_pointer_cast<BasicBlock>(now_BB->shared_from_this()));
    // }
    // LOG_DEBUG<<label_before_loop_pre->print();   
    auto insert_bb = BasicBlock::create(module, "temp_insertion_bb", main_func.get());  
    auto label_br = label_before_loop_pre->get_terminator_itr();
    LOG_DEBUG<<label_before_loop_pre->print();
    //删除原来的跳转指令
    label_before_loop_pre->delete_instr(label_br);
    //生成新的跳转指令
    auto new_br = BranchInst::create_br(insert_bb,label_before_loop_pre);
    
    builder->set_insert_point(insert_bb);  // TODO we have to get the place to insert
    scope.enter();
    // if (node.type != TYPE_VOID) {
    //     ret_val = builder->create_alloca(ret_type);
    //     node.type == TYPE_INT ? builder->create_store(CONST_INT(0), ret_val)
    //                           : builder->create_store(CONST_FP(0), ret_val);
    // }
    // pre_enter_scope = true;
    // std::vector<std::shared_ptr<Value>> args;
    // int i = 0;
    // for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++, i++) {
    //     auto param_alloc = builder->create_alloca(param_types[i]);
    //     builder->create_store(*arg, param_alloc);
    //     scope.push(node.param_list[i]->id, param_alloc);
    // }
    // ret_bb = BasicBlock::create(module, "return", cur_func);
    node.block->accept(*this);
    for (auto alloca_inst : alloca_instructions) {
        alloca_inst->set_parent(insert_bb.get());
        insert_bb->add_instr_begin(alloca_inst);
    }
    alloca_instructions.clear();
    builder->create_br(std::dynamic_pointer_cast<BasicBlock>(label_exit_loop_succ->shared_from_this()));

    // if (not builder->get_insert_block()->get_terminator())
        
    // builder->set_insert_point(ret_bb);
    // if (node.type == TYPE_VOID)
    //     builder->create_void_ret();
    // else {
    //     auto _ret = builder->create_load(ret_val);
    //     builder->create_ret(_ret);
    // }
    // scope.exit();
}     

std::map<Instruction *, std::shared_ptr<Value>> iv2actual;
std::unordered_set<Value *> poly_stmt_processed;
Loop *original_loop;
void PolyBuilder::replace_iv(ASTCall &call_node) {
    poly_stmt_processed.clear();
    std::vector<std::shared_ptr<Value>> actual_params;
    for (auto n : call_node.params) {
        n->accept(*this);
        actual_params.push_back(val);
    }
    LOG_DEBUG << "to replace: " << call_node.id;
    // auto main_func = *std::find_if(
        // module->get_functions().begin(), module->get_functions().end(), [](auto f) { return f->get_name() == "main"; });
    if (call_node.id == "min") {
        assert(actual_params.size() == 2);
        auto lhs = actual_params[0];
        auto rhs = actual_params[1];
        auto icmp = builder->create_icmp_lt(lhs, rhs);
        // auto insert_bb = BasicBlock::create(module, "temp_min_bb", main_func.get());
        val = builder->create_select(icmp, lhs, rhs);
        return;
    } else if (call_node.id == "max") {
        assert(actual_params.size() == 2);
        auto lhs = actual_params[0];
        auto rhs = actual_params[1];
        auto icmp = builder->create_icmp_gt(lhs, rhs);
        val = builder->create_select(icmp, lhs, rhs);
        return;
    }
    assert(name2stmt.count(call_node.id));
    auto stmt = name2stmt[call_node.id];
    assert(stmt->idxs.size() == actual_params.size());
    for (int i = 0; i < actual_params.size(); ++i)
        iv2actual[stmt->idxs[i]->inst.get()] = actual_params[i];
    original_loop = stmt->loop_;
    // code generation
    copy_stmt_recur(stmt->inst_.get());
}

/// \brief recursively copies statement in the original loop to current bb
void PolyBuilder::copy_stmt_recur(Instruction *inst) {
    // actually, quite like builder itself
    // we iterate through operands, visit them and update `val`
    if (iv2actual.count(inst)) {
        // induction variables of original loop
        val = iv2actual[inst];
        return;
    }
    if (original_loop->is_loop_invariant(inst)) {
        val = inst->shared_from_this();
        return;
    }
    std::vector<User::operand_t> new_operands;
    // can we implement a generic clone? too much work I guess
    for (auto op : inst->get_operands()) {
        if (isa<BasicBlock>(op.get())) {
            LOG_DEBUG << "TODO: got bb as operand, map to new bb";
            new_operands.push_back(std::weak_ptr(op->shared_from_this()));
        } else if (not isa<Instruction>(op.get())) {
            // constants, arguments
            new_operands.push_back(op->shared_from_this());
        } else {
            // instructions
            auto i = dynamic_cast<Instruction *>(op.get());
            copy_stmt_recur(i);
            new_operands.push_back(val);
        }
    }
    // clone inst, set `val`
    // type `Instruction` doesn't have ::create ...
    // check binary inst, gep, load and store first
    auto insert_bb = builder->get_insert_block().get();
    if (inst->isBinary()) {
        auto bin_inst = dynamic_cast<BinaryInst *>(inst);
        auto clone = BinaryInst::create(bin_inst->get_type(), bin_inst->get_instr_type(), insert_bb);
        for (int i = 0; i < new_operands.size(); ++i)
            clone->set_operand(i, new_operands[i]);
        val = clone;
        return;
    }
    if (inst->is_gep()) {
        auto gep_inst = dynamic_cast<GetElementPtrInst *>(inst);
        auto clone = GetElementPtrInst::create(gep_inst->get_type(), new_operands.size(), insert_bb);
        for (int i = 0; i < new_operands.size(); ++i)
            clone->set_operand(i, new_operands[i]);
        val = clone;
        return;
    }
    if (inst->is_load()) {
        auto load = dynamic_cast<LoadInst *>(inst);
        auto clone = LoadInst::create_load(load->get_type(), new_operands[0]->shared_from_this(), insert_bb);
        val = clone;
        return;
    }
    if (inst->is_store()) {
        // auto store = dynamic_cast<StoreInst *>(inst);
        auto clone = StoreInst::create_store(
            new_operands[0]->shared_from_this(), new_operands[1]->shared_from_this(), insert_bb);
        val = clone; // though we don't need it
        return;
    }

    switch (inst->get_instr_type()) {
        default:
            exit(POLY_CODEGEN_UNHANDLED_INST, "unsupported inst type: " + inst->get_instr_op_name());
    }
}

void PolyBuilder::visit(ASTLVal &now) {
    std::shared_ptr<Value> var(std::shared_ptr<Value>(scope.find(now.id)));
    if (not var) {
        LOG_DEBUG << now.id;
        auto boundv = poly_m->get_value(now.id);
        if (original_loop->is_loop_invariant(boundv))
            val = boundv->shared_from_this();
        else if (isa<Instruction>(boundv))
            copy_stmt_recur(dynamic_cast<Instruction *>(boundv));
        else {
            LOG_WARNING << "not loop invar, not inst: " << boundv->get_name();
            val = boundv->shared_from_this();
        }
        return;
    }
    // assert(var != nullptr);
    // assert(var->get_type()->is_pointer_type());

    // 简单变量
    if (var->get_type()->is_integer_type() or var->get_type()->is_float_type()) {
        LOG_WARNING << "lval node: constant type";
        val = var;
        return;
    }

    // 非数组变量处理
    if (now.index_list.size() == 0) {
        val = lval_as_rval ? builder->create_load(var) : var;
        return;
    }

    // 数组变量，生成若干 gep 指令
    bool tmp_lval_as_rval = lval_as_rval;
    lval_as_rval = true;
    for (int i = 0; i < now.index_list.size(); ++i) {
        // LOG_INFO << "lval node: " << var->print() << " type: " << var->get_type()->print();
        if (!(var->get_type()->is_pointer_type()))
            exit(146);
        now.index_list[i]->accept(*this);
        if (not val->get_type()->is_integer_type()) {
            exit(207);
            LOG_ERROR << "lval node: index is not integer";
            exit(ERROR_IN_GENERATING_IR);
        }
        val = convert(val, INT32_T);
        if (var->get_type()->get_pointer_element_type()->is_array_type()) {
            // DONE 添加 bool 类型转换的处理
            var = builder->create_gep(var, {CONST_INT(0), val});
        } else {
            // 指针类型
            if (!(var->get_type()->get_pointer_element_type()->is_pointer_type()))
                exit(147);
            var = builder->create_load(var);
            var = builder->create_gep(var, {val});
        }
    }
    lval_as_rval = tmp_lval_as_rval;
    // 作为右值时，为了获取值，需要额外的 load 指令
    val = lval_as_rval ? builder->create_load(var) : var;
}

void PolyBuilder::visit(ASTCompUnit &node) {
    VOID_T = Type::get_void_type(module);
    INT1_T = Type::get_int1_type(module);
    INT32_T = Type::get_int32_type(module);
    INT32PTR_T = Type::get_int32_ptr_type(module);
    FLOAT_T = Type::get_float_type(module);
    FLOATPTR_T = Type::get_float_ptr_type(module);
    empty_init_list = new ASTInitList;
    empty_const_init_list = new ASTConstInitList;
    for (auto decl_func : node.decl_func_list) {
        decl_func->accept(*this);
    }
    delete empty_init_list;
    delete empty_const_init_list;
}

void PolyBuilder::visit(ASTConstDecl &node) {
    if (node.type == TYPE_INT)
        cur_arr_inner_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        cur_arr_inner_type = FLOAT_T;
    for (auto def : node.const_def_list)
        def->accept(*this);
}

void PolyBuilder::visit(ASTConstDef &node) {
    Type *var_type = cur_arr_inner_type;
    cur_arr_shape.clear();
    // cur_init_index.resize(node.array_size.size());
    cur_arr_shape.resize(node.array_size.size());
    for (int i = node.array_size.size() - 1; i >= 0; i--) {
        node.array_size[i]->accept(*this);
        int size = static_cast<ConstantInt *>(val.get())->get_value();
        var_type = ArrayType::get(var_type, size);
        cur_arr_shape[i] = size;
        // cur_init_index[i] = 0;
    }
    if (node.array_size.empty()) {
        node.const_init_val->accept(*this);
        scope.push(node.id, val);
    } else {
        if (scope.in_global()) {
            glo_init.clear();
            glo_init.resize(node.array_size.size());
            cur_dim = 0;
            std::shared_ptr<Constant> initializer;
            if (node.const_init_val) {
                glo_init_finished.resize(cur_arr_shape.size());
                glo_init_finished.assign(glo_init_finished.size(), 0);
                node.const_init_val->accept(*this);
                initializer = global_initializer;
            } else
                initializer = ConstantZero::get(var_type, module);
            val = GlobalVariable::create(node.id, module, var_type, true, initializer);
            scope.push(node.id, val);
        } else {
            // val = builder->create_alloca(var_type);
            // builder->get_insert_block()->delete_instr(val);
            val = AllocaInst::create_alloca(var_type, nullptr);
            alloca_instructions.push_front(std::static_pointer_cast<AllocaInst>(val));
            scope.push(node.id, val);
            cur_dim = 0;
            if (node.const_init_val) {
                node.const_init_val->accept(*this);
            }
        }
    }
}

void PolyBuilder::visit(ASTConstInitSingle &node) {
    auto var = val;
    node.exp->accept(*this);
    val = type_cast(val, cur_arr_inner_type, *builder, module);
    if (scope.in_global()) {
        global_initializer = std::static_pointer_cast<Constant>(val);
    } else {
    }
}

void PolyBuilder::visit(ASTConstInitList &node) {
    if (!scope.in_global()) {
        bool braces_around_scalar_initializer = false;
        if (!val->get_type()->get_pointer_element_type()->is_array_type())
            braces_around_scalar_initializer = true;
        else
            val = builder->create_gep(val, {CONST_INT(0), CONST_INT(0)});
        // auto var = val;
        std::vector<std::shared_ptr<Value>> dim_trace;
        dim_trace.push_back(val);
        auto shape = cur_arr_shape;
        std::vector<int> index(shape.size(), 0);
        for (int i = 0; i < node.init_list.size(); i++) {
            auto init_list = dynamic_cast<ASTConstInitList *>(node.init_list[i].get());
            if (init_list == nullptr) {
                // auto single = static_cast<ASTConstInitSingle *>(node.init_list[i].get());
                // 取得最内层的元素
                for (int d = dim_trace.size(); d < shape.size(); d++) {
                    val = builder->create_gep(val, {CONST_INT(0), CONST_INT(0)});
                    dim_trace.push_back(val);
                }
                auto addr = val;
                node.init_list[i]->accept(*this);
                builder->create_store(val, addr);
                if (braces_around_scalar_initializer)
                    return;
            } else {
                cur_arr_shape.resize(shape.size() - dim_trace.size());
                if (cur_arr_shape.size() > 0)
                    cur_arr_shape[0] = shape[dim_trace.size()] - index[dim_trace.size()];
                for (int j = dim_trace.size() + 1; j < cur_arr_shape.size(); j++) {
                    cur_arr_shape[j] = shape[j];
                }
                init_list->accept(*this);
                if (braces_around_scalar_initializer)
                    return;
            }
            while (true) {
                if (dim_trace.empty())
                    return;
                val = dim_trace.back();
                dim_trace.pop_back();
                if (index[dim_trace.size()] < shape[dim_trace.size()] - 1)
                    break;
            }
            val = builder->create_gep(val, {CONST_INT(1)});
            index[dim_trace.size()]++;
            dim_trace.push_back(val);
        }
        while (!dim_trace.empty()) {
            int cur_dim_bound = shape[dim_trace.size() - 1];
            int cur_dim_index = index[dim_trace.size() - 1];
            val = dim_trace.back();
            auto bound_ptr = builder->create_gep(val, {CONST_INT(cur_dim_bound - cur_dim_index)});
            auto init_loop = BasicBlock::create(module, "initloop" + std::to_string(counter++), cur_func);
            auto init_finished = BasicBlock::create(module, "initfinish" + std::to_string(counter++), cur_func);
            builder->create_br(init_loop);
            auto start_ptr = val;
            auto pre_bb = builder->get_insert_block();
            builder->set_insert_point(init_loop);
            auto new_phi = PhiInst::create_phi(start_ptr->get_type());
            if (new_phi->get_type()->get_pointer_element_type()->is_array_type()) {
                // val = builder->create_gep(new_phi, {CONST_INT(0), CONST_INT(0)});
                cur_arr_shape.resize(shape.size() - dim_trace.size());
                if (cur_arr_shape.size() > 0)
                    cur_arr_shape[0] = shape[dim_trace.size()] - index[dim_trace.size()];
                for (int j = dim_trace.size() + 1; j < shape.size(); j++) {
                    cur_arr_shape[j - dim_trace.size()] = shape[j];
                }
                empty_const_init_list->accept(*this);
            } else {
                if (new_phi->get_type()->get_pointer_element_type()->is_float_type())
                    builder->create_store(CONST_FP(0), new_phi);
                else
                    builder->create_store(CONST_INT(0), new_phi);
            }
            auto cur_ptr = builder->create_gep(new_phi, {CONST_INT(1)});
            auto finished = builder->create_icmp_eq(cur_ptr, bound_ptr);
            builder->create_cond_br(finished, init_finished, init_loop);
            // init_loop->delete_instr(new_phi);
            new_phi->add_phi_pair_operand(start_ptr, pre_bb);
            new_phi->add_phi_pair_operand(std::weak_ptr(cur_ptr), builder->get_insert_block());
            init_loop->add_instr_begin(new_phi);
            new_phi->set_parent(init_loop.get());
            builder->set_insert_point(init_finished);
            dim_trace.pop_back();
            while (true) {
                if (dim_trace.empty())
                    return;
                val = dim_trace.back();
                dim_trace.pop_back();
                if (index[dim_trace.size()] < shape[dim_trace.size()] - 1)
                    break;
            }
            val = builder->create_gep(val, {CONST_INT(1)});
            index[dim_trace.size()]++;
            dim_trace.push_back(val);
        }
    } else {
        if (cur_dim < 0)
            LOG_ERROR << "error: global variable initializer error\n";
        int dealing_dim = cur_dim;
        int begin_dim = cur_dim;
        for (int i = 0; i < node.init_list.size(); i++) {
            auto init_list = dynamic_cast<ASTConstInitList *>(node.init_list[i].get());
            if (init_list == nullptr) {
                dealing_dim = cur_arr_shape.size() - 1;
                node.init_list[i]->accept(*this);
                glo_init[dealing_dim].push_back(global_initializer);
                glo_init_finished[dealing_dim]++;
                if (begin_dim >= cur_arr_shape.size())
                    return;
            } else {
                cur_dim = dealing_dim + 1;
                init_list->accept(*this);
                if (begin_dim >= cur_arr_shape.size())
                    return;
            }

            // 将填满的行组装成数组
            while (true) {
                if (dealing_dim < begin_dim)
                    return;
                if (glo_init_finished[dealing_dim] < cur_arr_shape[dealing_dim])
                    break;
                // auto arr_type = ArrayType::get(glo_init[dealing_dim][0]->get_type(), cur_arr_shape[dealing_dim]);
                // global_initializer = ConstantArray::get(arr_type, glo_init[dealing_dim]);
                global_initializer = build_up_initializer(glo_init[dealing_dim]);
                glo_init_finished[dealing_dim] = 0;
                dealing_dim--;
                if (dealing_dim >= 0) {
                    glo_init[dealing_dim].push_back(global_initializer);
                    glo_init_finished[dealing_dim]++;
                }
                glo_init[dealing_dim + 1].clear();
            }
        }
        while (dealing_dim >= begin_dim) {
            if (dealing_dim < cur_arr_shape.size() - 1) {
                // cur_dim = dealing_dim + 1;
                // empty_init_list->accept(*this);
                int rest_num = cur_arr_shape[dealing_dim] - glo_init_finished[dealing_dim];
                Type *rest_type = cur_arr_inner_type;
                for (int i = cur_arr_shape.size() - 1; i > dealing_dim; i--) {
                    rest_type = ArrayType::get(rest_type, cur_arr_shape[i]);
                }
                if (rest_num != 1)
                    rest_type = ArrayType::get(rest_type, rest_num);
                glo_init[dealing_dim].push_back(ConstantZero::get(rest_type, module));
                glo_init_finished[dealing_dim] = cur_arr_shape[dealing_dim];
            } else {
                int rest_num = cur_arr_shape[dealing_dim] - glo_init_finished[dealing_dim];
                if (rest_num == 1) {
                    if (cur_arr_inner_type == INT32_T)
                        glo_init[dealing_dim].push_back(CONST_INT(0));
                    else
                        glo_init[dealing_dim].push_back(CONST_FP(0));
                } else if (rest_num > 1) {
                    glo_init[dealing_dim].push_back(
                        ConstantZero::get(ArrayType::get(cur_arr_inner_type, rest_num), module));
                }
                glo_init_finished[dealing_dim] = cur_arr_shape[dealing_dim];
            }
            while (glo_init_finished[dealing_dim] == cur_arr_shape[dealing_dim]) {
                // auto arr_type = ArrayType::get(glo_init[dealing_dim][0]->get_type(), cur_arr_shape[dealing_dim]);
                if (node.init_list.size() > 0)
                    global_initializer = build_up_initializer(glo_init[dealing_dim]);
                else
                    global_initializer = glo_init[dealing_dim][0];
                glo_init_finished[dealing_dim] = 0;
                dealing_dim--;
                if (dealing_dim >= 0) {
                    glo_init[dealing_dim].push_back(global_initializer);
                    glo_init_finished[dealing_dim]++;
                }
                glo_init[dealing_dim + 1].clear();
                if (dealing_dim < begin_dim)
                    break;
            }
        }
    }
}

void PolyBuilder::visit(ASTVarDecl &node) {
    if (node.type == TYPE_INT)
        cur_arr_inner_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        cur_arr_inner_type = FLOAT_T;
    for (auto def : node.var_def_list)
        def->accept(*this);
}

void PolyBuilder::visit(ASTVarDef &node) {
    Type *var_type = cur_arr_inner_type;
    cur_arr_shape.clear();
    // cur_init_index.resize(node.array_size.size());
    cur_arr_shape.resize(node.array_size.size());
    for (int i = node.array_size.size() - 1; i >= 0; i--) {
        node.array_size[i]->accept(*this);
        int size = static_cast<ConstantInt *>(val.get())->get_value();
        var_type = ArrayType::get(var_type, size);
        cur_arr_shape[i] = size;
        // cur_init_index[i] = 0;
    }
    if (scope.in_global()) {
        glo_init.clear();
        glo_init.resize(node.array_size.size());
        cur_dim = 0;
        std::shared_ptr<Constant> initializer;
        if (node.init_val) {
            glo_init_finished.resize(cur_arr_shape.size());
            glo_init_finished.assign(glo_init_finished.size(), 0);
            node.init_val->accept(*this);
            initializer = global_initializer;
        } else
            initializer = ConstantZero::get(var_type, module);
        val = GlobalVariable::create(node.id, module, var_type, false, initializer);
        scope.push(node.id, val);
    } else {
        // val = builder->create_alloca(var_type);
        // builder->get_insert_block()->delete_instr(val);
        val = AllocaInst::create_alloca(var_type, nullptr);
        alloca_instructions.push_front(std::static_pointer_cast<AllocaInst>(val));
        scope.push(node.id, val);
        cur_dim = 0;
        cur_init_base = val;
        if (node.init_val) {
            node.init_val->accept(*this);
        }
    }
}

void PolyBuilder::visit(ASTInitSingle &node) {
    auto var = val;
    node.exp->accept(*this);
    val = type_cast(val, cur_arr_inner_type, *builder, module);
    if (scope.in_global()) {
        global_initializer = std::static_pointer_cast<Constant>(val);
    } else {
        builder->create_store(val, var);
    }
}

void PolyBuilder::visit(ASTInitList &node) {
    if (!scope.in_global()) {
        bool braces_around_scalar_initializer = false;
        if (!val->get_type()->get_pointer_element_type()->is_array_type())
            braces_around_scalar_initializer = true;
        else
            val = builder->create_gep(val, {CONST_INT(0), CONST_INT(0)});
        // auto var = val;
        std::vector<std::shared_ptr<Value>> dim_trace;
        dim_trace.push_back(val);
        auto shape = cur_arr_shape;
        std::vector<int> index(shape.size(), 0);
        for (int i = 0; i < node.init_list.size(); i++) {
            auto init_list = dynamic_cast<ASTInitList *>(node.init_list[i].get());
            if (init_list == nullptr) {
                // auto single = static_cast<ASTInitSingle *>(node.init_list[i].get());
                // 取得最内层的元素
                for (int d = dim_trace.size(); d < shape.size(); d++) {
                    val = builder->create_gep(val, {CONST_INT(0), CONST_INT(0)});
                    dim_trace.push_back(val);
                }
                node.init_list[i]->accept(*this);
                if (braces_around_scalar_initializer)
                    return;
            } else {
                cur_arr_shape.resize(shape.size() - dim_trace.size());
                if (cur_arr_shape.size() > 0)
                    cur_arr_shape[0] = shape[dim_trace.size()] - index[dim_trace.size()];
                for (int j = 1; j < cur_arr_shape.size(); j++) {
                    cur_arr_shape[j] = shape[j + dim_trace.size()];
                }
                init_list->accept(*this);
                if (braces_around_scalar_initializer)
                    return;
            }
            while (true) {
                if (dim_trace.empty())
                    return;
                val = dim_trace.back();
                dim_trace.pop_back();
                if (index[dim_trace.size()] < shape[dim_trace.size()] - 1)
                    break;
                else
                    index[dim_trace.size()] = 0;
            }
            val = builder->create_gep(val, {CONST_INT(1)});
            index[dim_trace.size()]++;
            dim_trace.push_back(val);
        }
        while (!dim_trace.empty()) {
            int cur_dim_bound = shape[dim_trace.size() - 1];
            int cur_dim_index = index[dim_trace.size() - 1];
            val = dim_trace.back();
            if (shape.size() == dim_trace.size() && cur_dim_bound - cur_dim_index <= 40) {
                for (int idx = 0; idx < cur_dim_bound - cur_dim_index; idx++) {
                    // std::vector<std::shared_ptr<Value>> idxs;
                    // idxs.push_back(CONST_INT(0));
                    // for (int i = 0; i < shape.size() - 1; i++) {
                    //     idxs.push_back(CONST_INT(index[i]));
                    // }
                    // idxs.push_back(CONST_INT(cur_dim_index + idx));
                    // auto ptr = builder->create_gep(cur_init_base, idxs);
                    auto ptr = builder->create_gep(val, {CONST_INT(idx)});
                    builder->create_store(type_cast(CONST_INT(0), cur_arr_inner_type, *builder, module), ptr);
                }
            } else {
                auto bound_ptr = builder->create_gep(val, {CONST_INT(cur_dim_bound - cur_dim_index)});
                auto init_loop = BasicBlock::create(module, "initloop" + std::to_string(counter++), cur_func);
                auto init_finished = BasicBlock::create(module, "initfinish" + std::to_string(counter++), cur_func);
                builder->create_br(init_loop);
                auto start_ptr = val;
                auto pre_bb = builder->get_insert_block();
                builder->set_insert_point(init_loop);
                auto new_phi = PhiInst::create_phi(start_ptr->get_type());
                if (new_phi->get_type()->get_pointer_element_type()->is_array_type()) {
                    // val = builder->create_gep(new_phi, {CONST_INT(0), CONST_INT(0)});
                    cur_arr_shape.resize(shape.size() - dim_trace.size());
                    if (cur_arr_shape.size() > 0)
                        cur_arr_shape[0] = shape[dim_trace.size()] - index[dim_trace.size()];
                    for (int j = dim_trace.size() + 1; j < shape.size(); j++) {
                        cur_arr_shape[j - dim_trace.size()] = shape[j];
                    }
                    val = new_phi;
                    empty_init_list->accept(*this);
                } else {
                    builder->create_store(type_cast(CONST_INT(0), cur_arr_inner_type, *builder, module), new_phi);
                }
                auto cur_ptr = builder->create_gep(new_phi, {CONST_INT(1)});
                auto finished = builder->create_icmp_eq(cur_ptr, bound_ptr);
                builder->create_cond_br(finished, init_finished, init_loop);
                // init_loop->delete_instr(new_phi);
                new_phi->add_phi_pair_operand(start_ptr, pre_bb);
                new_phi->add_phi_pair_operand(std::weak_ptr(cur_ptr), builder->get_insert_block());
                init_loop->add_instr_begin(new_phi);
                new_phi->set_parent(init_loop.get());
                builder->set_insert_point(init_finished);
            }
            dim_trace.pop_back();
            while (true) {
                if (dim_trace.empty())
                    return;
                val = dim_trace.back();
                dim_trace.pop_back();
                if (index[dim_trace.size()] < shape[dim_trace.size()] - 1)
                    break;
            }
            val = builder->create_gep(val, {CONST_INT(1)});
            index[dim_trace.size()]++;
            dim_trace.push_back(val);
        }
    } else {
        if (cur_dim < 0)
            LOG(ERROR) << "error: global variable initializer error\n";
        int dealing_dim = cur_dim;
        int begin_dim = cur_dim;
        for (int i = 0; i < node.init_list.size(); i++) {
            auto init_list = dynamic_cast<ASTInitList *>(node.init_list[i].get());
            if (init_list == nullptr) {
                dealing_dim = cur_arr_shape.size() - 1;
                node.init_list[i]->accept(*this);
                glo_init[dealing_dim].push_back(global_initializer);
                glo_init_finished[dealing_dim]++;
                if (begin_dim >= cur_arr_shape.size())
                    return;
            } else {
                cur_dim = dealing_dim + 1;
                init_list->accept(*this);
                if (begin_dim >= cur_arr_shape.size())
                    return;
            }

            // 将填满的行组装成数组
            while (true) {
                if (dealing_dim < begin_dim)
                    return;
                if (glo_init_finished[dealing_dim] < cur_arr_shape[dealing_dim])
                    break;
                // auto arr_type = ArrayType::get(glo_init[dealing_dim][0]->get_type(), cur_arr_shape[dealing_dim]);
                // global_initializer = ConstantArray::get(arr_type, glo_init[dealing_dim]);
                global_initializer = build_up_initializer(glo_init[dealing_dim]);
                glo_init_finished[dealing_dim] = 0;
                dealing_dim--;
                if (dealing_dim >= 0) {
                    glo_init[dealing_dim].push_back(global_initializer);
                    glo_init_finished[dealing_dim]++;
                }
                glo_init[dealing_dim + 1].clear();
            }
        }
        while (dealing_dim >= begin_dim) {
            if (dealing_dim < cur_arr_shape.size() - 1) {
                // cur_dim = dealing_dim + 1;
                // empty_init_list->accept(*this);
                int rest_num = cur_arr_shape[dealing_dim] - glo_init_finished[dealing_dim];
                Type *rest_type = cur_arr_inner_type;
                for (int i = cur_arr_shape.size() - 1; i > dealing_dim; i--) {
                    rest_type = ArrayType::get(rest_type, cur_arr_shape[i]);
                }
                if (rest_num != 1)
                    rest_type = ArrayType::get(rest_type, rest_num);
                glo_init[dealing_dim].push_back(ConstantZero::get(rest_type, module));
                glo_init_finished[dealing_dim] = cur_arr_shape[dealing_dim];
            } else {
                int rest_num = cur_arr_shape[dealing_dim] - glo_init_finished[dealing_dim];
                if (rest_num == 1) {
                    if (cur_arr_inner_type == INT32_T)
                        glo_init[dealing_dim].push_back(CONST_INT(0));
                    else
                        glo_init[dealing_dim].push_back(CONST_FP(0));
                } else if (rest_num > 1) {
                    glo_init[dealing_dim].push_back(
                        ConstantZero::get(ArrayType::get(cur_arr_inner_type, rest_num), module));
                }
                glo_init_finished[dealing_dim] = cur_arr_shape[dealing_dim];
            }
            while (glo_init_finished[dealing_dim] == cur_arr_shape[dealing_dim]) {
                // auto arr_type = ArrayType::get(glo_init[dealing_dim][0]->get_type(), cur_arr_shape[dealing_dim]);
                if (node.init_list.size() > 0)
                    global_initializer = build_up_initializer(glo_init[dealing_dim]);
                else
                    global_initializer = glo_init[dealing_dim][0];
                glo_init_finished[dealing_dim] = 0;
                dealing_dim--;
                if (dealing_dim >= 0) {
                    glo_init[dealing_dim].push_back(global_initializer);
                    glo_init_finished[dealing_dim]++;
                }
                glo_init[dealing_dim + 1].clear();
                if (dealing_dim < begin_dim)
                    break;
            }
        }
    }
}

void PolyBuilder::visit(ASTFuncFParam &node) {
    if (node.type == TYPE_INT)
        cur_arr_inner_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        cur_arr_inner_type = FLOAT_T;
    if (node.is_array) {
        for (int arr_ = node.array_exp_list.size() - 1; arr_ >= 0; arr_--) {
            node.array_exp_list[arr_]->accept(*this);
            cur_arr_inner_type = ArrayType::get(cur_arr_inner_type, static_cast<ConstantInt *>(val.get())->get_value());
        }
        cur_arr_inner_type = PointerType::get(cur_arr_inner_type);
    }
}

void PolyBuilder::visit(ASTBlock &node) {
    if (block_depth > 0)
        scope.enter();
    block_depth++;
    for (auto item : node.item_list) {
        item->accept(*this);
    }
    block_depth--;
    if (block_depth > 0)
        scope.exit();
}

void PolyBuilder::visit(ASTBlockItem &node) {
    if (node.const_decl) {
        node.const_decl->accept(*this);
        return;
    } else if (node.var_decl) {
        node.var_decl->accept(*this);
        return;
    } else if (node.stmt) {
        node.stmt->accept(*this);
        return;
    }
}

void PolyBuilder::visit(ASTAssignStmt &node) {
    bool tmp = lval_as_rval;
    lval_as_rval = false;
    node.lval->accept(*this);
    lval_as_rval = tmp;

    auto dest = val;
    node.exp->accept(*this);
    val = type_cast(val, dest->get_type()->get_pointer_element_type(), *builder, module);
    builder->create_store(val, dest);
}

void PolyBuilder::visit(ASTExpStmt &node) {
    if (node.exp)
        node.exp->accept(*this);
}

/// \brief shortcut circuit
void PolyBuilder::visit(ASTSelectionStmt &s) {
    auto t = true_block = BasicBlock::create(&*module, "", cur_func);
    auto e = false_block = BasicBlock::create(&*module, "", cur_func);
    s.cond->accept(*this);
    // move to end by hand
    cur_func->get_basic_blocks().remove(t), cur_func->get_basic_blocks().push_back(t);
    cur_func->get_basic_blocks().remove(e), cur_func->get_basic_blocks().push_back(e);
    t->set_name("then" + std::to_string(counter++));
    e->set_name("else" + std::to_string(counter++));
    builder->set_insert_point(t);
    s.true_stmt->accept(*this);
    auto out = s.false_stmt ? BasicBlock::create(&*module, "out" + std::to_string(counter++), cur_func) : e;
    if (not builder->get_insert_block()->get_terminator())
        builder->create_br(out);
    builder->set_insert_point(e);
    if (s.false_stmt) {
        s.false_stmt->accept(*this);
        if (not builder->get_insert_block()->get_terminator())
            builder->create_br(out);
        builder->set_insert_point(out);
    }
}

/// \details check validity of condition first, useful for loop invar motion. i.e.
/// if cond then
/// do
///     loop
/// until not cond
void PolyBuilder::visit(ASTWhileStmt &s) {
    auto old_break_bb = break_bb;
    auto old_continue_bb = continue_bb;
    auto before_loop = BasicBlock::create(&*module, "beforeloop" + std::to_string(counter++), cur_func);
    auto pre_header = true_block = BasicBlock::create(&*module, "preheader" + std::to_string(counter++), cur_func);
    auto body = BasicBlock::create(&*module, "body" + std::to_string(counter++), cur_func);
    auto latch = continue_bb = BasicBlock::create(&*module, "latch" + std::to_string(counter++), cur_func);
    auto afterloop = false_block = BasicBlock::create(&*module, "afterloop" + std::to_string(counter++), cur_func);
    auto exit = break_bb = BasicBlock::create(&*module, "exit" + std::to_string(counter++), cur_func);
    builder->create_br(before_loop);
    builder->set_insert_point(before_loop);
    // cond for if statement
    s.cond->accept(*this);
    builder->set_insert_point(pre_header);
    builder->create_br(body);
    builder->set_insert_point(body);
    s.stmt->accept(*this);
    // cond for while loop
    true_block = body;
    false_block = exit;
    builder->create_br(latch);
    builder->set_insert_point(latch);
    s.cond->accept(*this);
    builder->set_insert_point(exit);
    builder->create_br(afterloop);
    builder->set_insert_point(afterloop);
    break_bb = old_break_bb;
    continue_bb = old_continue_bb;
}

void PolyBuilder::visit(ASTBreakStmt &s) { builder->create_br(break_bb); }
void PolyBuilder::visit(ASTContinueStmt &) { builder->create_br(continue_bb); }

void PolyBuilder::visit(ASTReturnStmt &s) {
    // TODO: 先弄成 in place 的 return，之后再搞成跳到统一的 return block
    if (s.exp) {
        s.exp->accept(*this);
        builder->create_store(type_cast(val, cur_func->get_function_type()->get_return_type(), *builder, module),
                              ret_val);
    }
    builder->create_br(ret_bb);
}

void PolyBuilder::visit(ASTForStmt &s) {
    // loop structure: beforeloop, preheaders, body, latch, exit
    LOG_DEBUG << "Poly IR inserter: For Statement";
    // s.var->accept(*this);  // it shouldn't be lval... it's a decl
    // but whatever
    auto newval = val = AllocaInst::create_alloca(INT32_T, nullptr);
    alloca_instructions.push_front(std::static_pointer_cast<AllocaInst>(val));
    scope.push(s.var->id, val);

    s.init->accept(*this);

    builder->create_store(val, newval);
    // setting up loop contents
    auto old_break_bb = break_bb;
    auto old_continue_bb = continue_bb;

    auto before_loop = BasicBlock::create(&*module, "beforeloop" + std::to_string(counter++), cur_func);
    auto pre_header = true_block = BasicBlock::create(&*module, "preheader" + std::to_string(counter++), cur_func);
    auto body = BasicBlock::create(&*module, "body" + std::to_string(counter++), cur_func);
    auto latch = continue_bb = BasicBlock::create(&*module, "latch" + std::to_string(counter++), cur_func);
    auto afterloop = false_block = BasicBlock::create(&*module, "afterloop" + std::to_string(counter++), cur_func);
    auto exit = break_bb = BasicBlock::create(&*module, "exit" + std::to_string(counter++), cur_func);
    builder->create_br(before_loop);
    builder->set_insert_point(before_loop);

    s.cond->accept(*this);

    builder->set_insert_point(pre_header);
    builder->create_br(body);
    builder->set_insert_point(body);

    s.stmt->accept(*this);  // loop body
    s.step->accept(*this);
    auto lval_load = builder->create_load(newval);
    auto add_step = builder->create_iadd(lval_load, val);
    builder->create_store(add_step,newval);
    true_block = body;
    false_block = exit;
    builder->create_br(latch);
    builder->set_insert_point(latch);
    s.cond->accept(*this);
    builder->set_insert_point(exit);
    builder->create_br(afterloop);
    builder->set_insert_point(afterloop);
    break_bb = old_break_bb;
    continue_bb = old_continue_bb;
}

void PolyBuilder::visit(ASTOpUnaryExp &s) {
    // Type *ty;
    switch (s.op) {
        case UnaryOp::OP_POS:
            s.unary_exp->accept(*this);
            break;
        case UnaryOp::OP_NEG:
            s.unary_exp->accept(*this);
            val = calculate(CONST_INT(0), val, bin_op::sub);
            break;
        case UnaryOp::OP_NOT:
            std::swap(true_block, false_block);
            s.unary_exp->accept(*this);
            break;
    }
}

void PolyBuilder::visit(ASTLOrExp &s) {
    if (s.lor_exp == nullptr) {
        s.land_exp->accept(*this);
    } else {
        // saving true & false block
        auto t = true_block;
        auto e = false_block;
        auto orbb = false_block = BasicBlock::create(&*module, "lor" + std::to_string(counter++), cur_func);
        s.lor_exp->accept(*this);
        true_block = t;
        false_block = e;
        builder->set_insert_point(orbb);
        s.land_exp->accept(*this);
    }
}

void PolyBuilder::visit(ASTLAndExp &s) {
    branch = true;
    if (s.land_exp == nullptr) {
        s.eq_exp->accept(*this);
    } else {
        auto t = true_block;
        auto e = false_block;
        auto andbb = true_block = BasicBlock::create(&*module, "land" + std::to_string(counter++), cur_func);
        s.land_exp->accept(*this);
        true_block = t;
        false_block = e;
        builder->set_insert_point(andbb);
        branch = true;
        s.eq_exp->accept(*this);
    }
}

void PolyBuilder::visit(ASTEqExp &s) {
    bool br_save = branch;
    if (s.eq_exp == nullptr) {
        s.rel_exp->accept(*this);
        if (branch and val->get_type()->get_size() > 1) {
            if (val->get_type()->is_integer_type())
                val = builder->create_icmp_ne(val, ConstantInt::get(0, &*module));
            else if (val->get_type()->is_float_type())
                val = builder->create_fcmp_ne(val, ConstantFP::get(0, &*module));
            if (br_save)
                builder->create_cond_br(val, true_block, false_block);
        }
    } else {
        branch = false;
        s.eq_exp->accept(*this);
        auto lhs = val;
        branch = false;
        s.rel_exp->accept(*this);
        auto rhs = val;
        // check float
        if (lhs->get_type()->is_float_type() or rhs->get_type()->is_float_type()) {
            if (lhs->get_type()->is_integer_type())
                lhs = builder->create_sitofp(lhs, Type::get_float_type(module));
            if (rhs->get_type()->is_integer_type())
                rhs = builder->create_sitofp(rhs, Type::get_float_type(module));
            switch (s.op) {
                case EqOp::OP_EQ:
                    val = builder->create_fcmp_eq(lhs, rhs);
                    break;
                case EqOp::OP_NEQ:
                    val = builder->create_fcmp_ne(lhs, rhs);
                    break;
            }
        } else {
            if (lhs->get_type()->is_integer_type() and lhs->get_type()->get_size() == 1) {
                lhs = builder->create_zext(lhs, Type::get_int32_type(module));
            }
            if (rhs->get_type()->is_integer_type() and rhs->get_type()->get_size() == 1) {
                rhs = builder->create_zext(rhs, Type::get_int32_type(module));
            }
            switch (s.op) {
                case EqOp::OP_EQ:
                    val = builder->create_icmp_eq(lhs, rhs);
                    break;
                case EqOp::OP_NEQ:
                    val = builder->create_icmp_ne(lhs, rhs);
                    break;
            }
        }
        if (br_save)
            builder->create_cond_br(val, true_block, false_block);
    }
}

void PolyBuilder::visit(ASTRelExp &s) {
    bool branch_save = branch;
    if (s.rel_exp == nullptr) {
        s.add_exp->accept(*this);
    } else {
        branch = false;
        s.rel_exp->accept(*this);
        auto lhs = val;
        s.add_exp->accept(*this);
        auto rhs = val;
        if (lhs->get_type()->is_float_type() or rhs->get_type()->is_float_type()) {
            if (lhs->get_type()->is_integer_type())
                lhs = builder->create_sitofp(lhs, Type::get_float_type(module));
            if (rhs->get_type()->is_integer_type())
                rhs = builder->create_sitofp(rhs, Type::get_float_type(module));
            switch (s.op) {
                case RelOp::OP_LE:
                    val = builder->create_fcmp_le(lhs, rhs);
                    break;
                case RelOp::OP_LT:
                    val = builder->create_fcmp_lt(lhs, rhs);
                    break;
                case RelOp::OP_GT:
                    val = builder->create_fcmp_gt(lhs, rhs);
                    break;
                case RelOp::OP_GE:
                    val = builder->create_fcmp_ge(lhs, rhs);
                    break;
            }
        } else {
            if (lhs->get_type()->is_integer_type() and lhs->get_type()->get_size() == 1) {
                lhs = builder->create_zext(lhs, Type::get_int32_type(module));
            }
            if (rhs->get_type()->is_integer_type() and rhs->get_type()->get_size() == 1) {
                rhs = builder->create_zext(rhs, Type::get_int32_type(module));
            }

            switch (s.op) {
                case RelOp::OP_LE:
                    val = builder->create_icmp_le(lhs, rhs);
                    break;
                case RelOp::OP_LT:
                    val = builder->create_icmp_lt(lhs, rhs);
                    break;
                case RelOp::OP_GT:
                    val = builder->create_icmp_gt(lhs, rhs);
                    break;
                case RelOp::OP_GE:
                    val = builder->create_icmp_ge(lhs, rhs);
                    break;
            }
        }
        if (branch_save)
            builder->create_cond_br(val, true_block, false_block);
    }
}

void PolyBuilder::visit(ASTConstExp &s) {
    s.add_exp->accept(*this);
    return;
}

// qmf
void PolyBuilder::visit(ASTAddExp &now) {
    if (now.add_exp == nullptr) {
        now.mul_exp->accept(*this);
        return;
    }
    now.add_exp->accept(*this);
    auto lhs = val;
    now.mul_exp->accept(*this);
    auto rhs = val;

    auto lhs_type = lhs->get_type();
    auto rhs_type = rhs->get_type();

    if (lhs_type->is_pointer_type() and rhs_type->is_pointer_type()) {
        auto lhs_type1 = lhs_type->get_pointer_element_type();
        int lhs_type1_id = lhs_type1->get_type_id();

        auto lhs_type2 = lhs_type1->get_pointer_element_type();
        int lhs_type2_id = lhs_type2->get_type_id();

        bool lhs_global = dynamic_cast<GlobalVariable *>(lhs.get()) != nullptr;
        bool rhs_global = dynamic_cast<GlobalVariable *>(rhs.get()) != nullptr;

        LOG_DEBUG << "lhs_type1_id: " << lhs_type1_id << " lhs_type2_id: " << lhs_type2_id
                  << " lhs_global: " << lhs_global << " rhs_global: " << rhs_global;

        // 82
        // lhs_type1: PointerType
        // rhs_type1: IntegerType
        // exit(lhs_type1_id << 4 | rhs_type1_id);

        // 32
        // lhs_type2: IntegerType
        // lhs_global: false
        // rhs_global: false
        // exit(lhs_type2_id << 4 | lhs_global << 2 | rhs_global);
    }

    // 33
    // left_ptr_shape = 2
    // right_ptr_shape = 1
    // exit(left_ptr_shape << 4 | right_ptr_shape);

    switch (now.op) {
        case OP_PLUS:
            val = calculate(lhs, rhs, bin_op::add);
            return;
        case OP_MINUS:
            val = calculate(lhs, rhs, bin_op::sub);
            return;
        default:
            LOG_ERROR << "internal error: error while generating IR of add-exp"s;
    }
}

void PolyBuilder::visit(ASTMulExp &now) {
    if (now.mul_exp == nullptr) {
        now.unary_exp->accept(*this);
        return;
    }

    now.mul_exp->accept(*this);
    auto lhs = val;
    now.unary_exp->accept(*this);
    auto rhs = val;

    switch (now.op) {
        case OP_MUL:
            val = calculate(lhs, rhs, bin_op::mul);
            return;
        case OP_DIV:
            val = calculate(lhs, rhs, bin_op::div);
            return;
        case OP_MOD:
            val = calculate(lhs, rhs, bin_op::mod);
            return;
        default:
            LOG_ERROR << "internal error: error while generating IR of mul-exp"s;
    }
}

// TODO
void PolyBuilder::visit(ASTCall &now) {
    return replace_iv(now);
    auto func = std::dynamic_pointer_cast<Function>(scope.find(now.id));

    if (func == nullptr) {
        // LOG_ERROR << "syntax error: unable to find delcaration of function " << now.id << "\n";
    }
    if (now.params.size() != func->get_num_of_args()) {
        LOG_ERROR << "syntax error: number of params does not match\n";
    }

    std::vector<std::shared_ptr<Value>> args;
    auto it = func->arg_begin();
    for (int i = 0, size = func->get_num_of_args(); i < size; ++i, it++) {
        auto arg = now.params[i];

        debug_flag = 1;

        bool tmp = lval_as_rval;
        lval_as_rval = not(*it)->get_type()->is_pointer_type();
        arg->accept(*this);
        lval_as_rval = tmp;

        debug_flag = 0;

        // DONE 确定是否需要 address only 的选项
        // 默认既是获取值，且函数返回值不能作为左值出现，因此不需要额外的判断
        // 但是参数需要额外的转换，例如数组参数需要取地址
        auto arg_val = convert(val, (*it)->get_type());
        args.push_back(arg_val);
    }
    val = builder->create_call(func, args);
}

void PolyBuilder::visit(ASTNumber &now) {
    switch (now.type) {
        case TYPE_INT:
            val = ConstantInt::get(now.int_val, module);
            return;
        case TYPE_FLOAT:
            val = ConstantFP::get(now.float_val, module);
            return;
        default:
            LOG_ERROR << "internal error: type of number is neither int nor float\n";
    }
}

std::shared_ptr<Value> PolyBuilder::convert(std::shared_ptr<Value> now, Type *to) {
    LOG_DEBUG << "convert: " << now->get_type()->print() << " to " << to->print();

    if (now->get_type()->is_integer_type() and to->is_integer_type()) {
        if (now->get_type()->get_size() < to->get_size()) {
            return builder->create_zext(now, to);
        }
        return now;
    }

    if (now->get_type() == to)
        return now;

    if (now->get_type()->is_integer_type() and to->is_float_type())
        return builder->create_sitofp(now, to);
    if (now->get_type()->is_float_type() and to->is_integer_type())
        return builder->create_fptosi(now, to);

    if (now->get_type()->is_pointer_type()) {
        if (now->get_type()->get_pointer_element_type()->is_array_type() and to->is_pointer_type() and
            now->get_type()->get_pointer_element_type()->get_array_element_type() == to->get_pointer_element_type())
            return builder->create_gep(now, {CONST_INT(0), CONST_INT(0)});

        // DONE 问一下这一段为什么要这么做
        // 这一串在以数组为参数进行函数调用时会触发
        if (now->get_type()->get_pointer_element_type()->is_pointer_type() and to->is_pointer_type()) {
            // LOG_WARNING << "conversion: unexpect conversion"
            //             << ", from " << now->print() << " to " << to->print();
            return builder->create_load(now);
        }
    }

    if (now->get_type()->is_array_type()) {
        exit(208);
        LOG_ERROR << "conversion: conversion array type";
        exit(ERROR_IN_GENERATING_IR);
    }

    LOG_ERROR << ("internal error: unexpected convert");
    exit(ERROR_IN_GENERATING_IR);
}

std::shared_ptr<Value> PolyBuilder::calculate(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, bin_op op) {
    // 常数表达式的处理
    auto const_lhs = std::dynamic_pointer_cast<Constant>(lhs);
    auto const_rhs = std::dynamic_pointer_cast<Constant>(rhs);
    if (const_lhs and const_rhs) {
        if (lhs->get_type()->is_float_type() or rhs->get_type()->is_float_type()) {
            float lval, rval;
            if (lhs->get_type()->is_integer_type())
                lval = static_cast<float>(std::dynamic_pointer_cast<ConstantInt>(const_lhs)->get_value());
            else
                lval = std::static_pointer_cast<ConstantFP>(const_lhs)->get_value();
            if (rhs->get_type()->is_integer_type())
                rval = static_cast<float>(std::dynamic_pointer_cast<ConstantInt>(const_rhs)->get_value());
            else
                rval = std::static_pointer_cast<ConstantFP>(const_rhs)->get_value();
            if (op == bin_op::add)
                return CONST_FP(lval + rval);
            else if (op == bin_op::sub)
                return CONST_FP(lval - rval);
            else if (op == bin_op::mul)
                return CONST_FP(lval * rval);
            else if (op == bin_op::div)
                return CONST_FP(lval / rval);
            else if (op == bin_op::mod)
                LOG_ERROR << "syntax error: mod between float number\n";
            LOG_ERROR << "internal error: error while generating IR of const float"s;
            exit(ERROR_IN_GENERATING_IR);
        } else {
            int lval, rval;
            lval = std::static_pointer_cast<ConstantInt>(lhs)->get_value();
            rval = std::static_pointer_cast<ConstantInt>(rhs)->get_value();
            if (op == bin_op::add)
                return CONST_INT(lval + rval);
            else if (op == bin_op::sub)
                return CONST_INT(lval - rval);
            else if (op == bin_op::mul)
                return CONST_INT(lval * rval);
            else if (op == bin_op::div)
                return CONST_INT(lval / rval);
            else if (op == bin_op::mod)
                return CONST_INT(lval % rval);
            LOG_ERROR << "internal error: error while generating IR of const integer"s;
            exit(ERROR_IN_GENERATING_IR);
        }
    }
    // bool 型的处理
    if (lhs->get_type()->is_integer_type() and lhs->get_type()->get_size() == 1) {
        lhs = builder->create_zext(lhs, Type::get_int32_type(module));
    }
    if (rhs->get_type()->is_integer_type() and rhs->get_type()->get_size() == 1) {
        rhs = builder->create_zext(rhs, Type::get_int32_type(module));
    }

    // 浮点型的处理
    if (lhs->get_type()->is_float_type() or rhs->get_type()->is_float_type()) {
        // 隐式类型转换
        // TODO: 等类型转换函数统一后改用转换函数
        if (lhs->get_type()->is_integer_type()) {
            lhs = builder->create_sitofp(lhs, Type::get_float_type(module));
        }
        if (rhs->get_type()->is_integer_type()) {
            rhs = builder->create_sitofp(rhs, Type::get_float_type(module));
        }

        if (op == bin_op::add)
            return builder->create_fadd(lhs, rhs);
        else if (op == bin_op::sub)
            return builder->create_fsub(lhs, rhs);
        else if (op == bin_op::mul)
            return builder->create_fmul(lhs, rhs);
        else if (op == bin_op::div)
            return builder->create_fdiv(lhs, rhs);
        else if (op == bin_op::mod)
            LOG_ERROR << "syntax error: mod between float number\n";
        LOG_ERROR << "internal error: error while generating IR of float"s;
        exit(ERROR_IN_GENERATING_IR);
    }

    // LOG_INFO << lhs->get_type()->print() << " " << rhs->get_type()->print();
    // LOG_INFO << lhs->get_type()->is_integer_type() << " " << rhs->get_type()->is_integer_type();
    // 两者均为 32bit 整数
    if (lhs->get_type()->is_integer_type() and rhs->get_type()->is_integer_type()) {
        if (op == bin_op::add)
            return builder->create_iadd(lhs, rhs);
        else if (op == bin_op::sub)
            return builder->create_isub(lhs, rhs);
        else if (op == bin_op::mul)
            return builder->create_imul(lhs, rhs);
        else if (op == bin_op::div)
            return builder->create_isdiv(lhs, rhs);
        else if (op == bin_op::mod)
            return builder->create_srem(lhs, rhs);
    }

    // fix load argument
    // TODO: 其他 数组 + 数字 的情况
    if (lhs->get_type()->is_pointer_type() and rhs->get_type()->is_pointer_type()) {
        auto lhs1 = builder->create_load(lhs);
        // auto lhs2 = builder->create_load(lhs1);
        auto rhs1 = builder->create_load(rhs);
        if (op == bin_op::add) {
            return builder->create_gep(lhs1, {rhs1});
            // return builder->create_iadd(lhs2, rhs1);
        }
    }

    int lhs_type_id = lhs->get_type()->get_type_id();
    int rhs_type_id = rhs->get_type()->get_type_id();
    int op_int = static_cast<int>(op);
    if (op_int >= 4)
        op_int = 3;

    LOG_DEBUG << "lhs_type: " << lhs->get_type()->print() << " rhs_type: " << rhs->get_type()->print()
              << " op: " << op_int;

    // 45
    // op_int = 0
    // lhs_type_id = 5
    // rhs_type_id = 5
    // 指针 + 指针
    // exit(op_int << 6 | lhs_type_id << 3 | rhs_type_id);

    LOG_ERROR << "internal error: error while generating IR of integer"s;
}

std::shared_ptr<Constant> PolyBuilder::build_up_initializer(std::vector<std::shared_ptr<Constant>> line) {
    bool isomorphic = true;
    for (auto ele : line) {
        if (ele->get_type() != line[0]->get_type()) {
            isomorphic = false;
            break;
        }
    }
    if (isomorphic) {
        //     if (line.size() != size) {
        //         exit(236);
        //         LOG_ERROR << "error while build up global initializer: line size doesn't match shape";
        //     }
        auto arr_ty = ArrayType::get(line[0]->get_type(), line.size());
        return ConstantArray::get(arr_ty, line);
    } else {
        std::vector<Type *> struct_ele_ty;
        for (auto ele : line) {
            struct_ele_ty.push_back(ele->get_type());
        }
        auto struct_ty = StructType::get(struct_ele_ty, module);
        return ConstantStruct::get(struct_ty, line);
    }
}

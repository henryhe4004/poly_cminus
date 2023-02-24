#include "AlgebraicSimplification.hh"

#include "DeadCode.hh"
#include "utils.hh"

#include <algorithm>
#include <cfenv>
#include <map>

bool test_reciprocal_exception(float c);
using alop = AlgebraicSimplification::ExpTreeNode::AlgebraicOp;

void AlgebraicSimplification::run() {
    // std::cout << "【【【【before】】】】\n";
    // std::cout << m_->print();
    LOG_INFO << "running algebraic simplification pass";
    for (auto func : m_->get_functions()) {
        if (func->is_declaration())
            continue;
        // 构建算术表达式树（森林）
        for (auto bb : func->get_basic_blocks()) {
            for (auto instr : bb->get_instructions()) {
                if (is_alge_inst(instr.get()))
                    update_tree(instr);
            }
        }
        // 对每棵树进行重结合操作
        for (auto root : roots) {
            reassociation(root);
        }
        // 使用新的Value（指令或常数）替换原指令
        replace_instruction(func);
        // for (auto bb : func->get_basic_blocks()) {
        //     for (auto instr_itr = bb->get_instructions().begin(); instr_itr != bb->get_instructions().end();
        //          instr_itr++) {
        //         auto instr = *instr_itr;
        //         if (is_alge_inst(instr.get())) {
        //             auto tree_node = value2node[instr.get()];
        //             if (tree_node == nullptr) {
        //                 exit(213);
        //                 LOG_ERROR << "an arithmetic instruction has no tree node";
        //                 continue;
        //             }
        //             auto new_val = tree_node->new_val;
        //             for (auto new_instr : tree_node->new_instrs) {
        //                 bb->get_instructions().insert(instr_itr, new_instr);
        //                 new_instr->set_parent(bb.get());
        //             }
        //             if (new_val) {
        //                 tree_node->val->replace_all_use_with(new_val);
        //                 tree_node->val->get_use_list().clear();
        //                 if (!tree_node->is_const) {
        //                     auto new_instr = std::static_pointer_cast<Instruction>(new_val);
        //                     bb->get_instructions().insert(instr_itr, new_instr);
        //                     new_instr->set_parent(bb.get());
        //                 }
        //             }
        //         }
        //     }
        // }

        // int_mul_weaken(func);
        // eliminate_invalid_operation(func);
        // auto del_instr = true;
        // while (del_instr) {
        //     del_instr = false;
        //     for (auto bb : func->get_basic_blocks()) {
        //         auto pre_instr = bb->get_instructions().begin();
        //         auto cur_instr = pre_instr;
        //         cur_instr++;
        //         while (cur_instr != bb->get_instructions().end()) {
        //             if ((*pre_instr)->isBinary() && (*pre_instr)->get_use_list().size() == 0) {
        //                 (*pre_instr)->remove_use_of_ops();
        //                 // LOG_DEBUG << "delete user " << (*pre_instr)->get_name();
        //                 bb->get_instructions().erase(pre_instr);
        //                 del_instr = true;
        //             }
        //             pre_instr = cur_instr;
        //             cur_instr++;
        //         }
        //     }
        // }
        roots.clear();
        value2node.clear();
    }
    DeadCode{m_}.run();
    // std::cout << "【【【after】】】\n";
    std::cout << m_->print();
}

void AlgebraicSimplification::replace_instruction(std::shared_ptr<Function> func) {
    for (auto bb : func->get_basic_blocks()) {
        // auto instr_itr = bb->get_instructions().end();
        // instr_itr--;
        int dbg = 0;
        for (auto instr_itr = bb->begin(); instr_itr != bb->end(); instr_itr++) {
            dbg++;
            if (dbg % 1000 == 0)
                LOG_DEBUG << dbg;
            auto instr = *instr_itr;
            if (is_alge_inst(instr.get())) {
                auto tree_node = value2node[instr.get()];
                if (tree_node != nullptr && tree_node->need_replace) {
                    std::map<int, std::vector<std::shared_ptr<Value>>, std::greater<int>> term_num;
                    for (auto term : tree_node->terms) {
                        if (term_num.find(term.second) == term_num.end())
                            term_num.insert({term.second, {term.first}});
                        else
                            term_num[term.second].push_back(term.first);
                    }
                    // if (term_num.size() == 1 && term_num.begin()->first == 1)
                    //     goto loop_end;
                    std::shared_ptr<Value> new_val = ConstantInt::get(0, m_);
                    for (auto terms : term_num) {
                        auto new_term = terms.second[0];
                        for (int i = 1; i < terms.second.size(); i++) {
                            new_term = utils::add_i32(new_term, terms.second[i], m_, &instr_itr);
                        }
                        if (terms.first > 0) {
                            new_term = utils::mul_i32(new_term, ConstantInt::get(terms.first, m_), m_, &instr_itr);
                            new_val = utils::add_i32(new_val, new_term, m_, &instr_itr);
                        } else {
                            new_term = utils::mul_i32(new_term, ConstantInt::get(-terms.first, m_), m_, &instr_itr);
                            new_val = utils::sub_i32(new_val, new_term, m_, &instr_itr);
                        }
                    }
                    new_val =
                        utils::add_i32(new_val, ConstantInt::get(tree_node->get_const_int_val(), m_), m_, &instr_itr);
                    if (new_val != instr)
                        instr->replace_all_use_with(new_val);
                    // DeadCode{m_}.run();
                    // instr_itr = bb->find_instr(instr);
                }
            }
            // loop_end:
            //     if (instr_itr == bb->get_instructions().begin())
            //         break;
            //     else
            //         instr_itr--;
        }
    }
}

std::shared_ptr<AlgebraicSimplification::ExpTreeNode> AlgebraicSimplification::get_node(std::shared_ptr<Value> val) {
    auto finded = value2node.find(val.get());
    if (finded != value2node.end())
        return finded->second;
    auto new_node = std::shared_ptr<ExpTreeNode>(new ExpTreeNode(val, nullptr, nullptr, alop::leaf));
    value2node[val.get()] = new_node;
    return new_node;
}

void AlgebraicSimplification::update_tree(std::shared_ptr<Instruction> instr) {
    auto exist_node = value2node.find(instr.get());
    if (exist_node == value2node.end()) {
        auto lhs = std::get<0>(instr->get_operand(0).operand);
        auto rhs = std::get<0>(instr->get_operand(1).operand);
        auto l_node = get_node(lhs);
        auto r_node = get_node(rhs);
        auto new_node = std::shared_ptr<ExpTreeNode>(new ExpTreeNode(instr, l_node, r_node));
        value2node[instr.get()] = new_node;
        roots.insert(new_node);
        roots.erase(l_node);
        roots.erase(r_node);
    } else {
        exist_node->second->set_op(instr->get_instr_type());
        auto lhs = std::get<0>(instr->get_operand(0).operand);
        auto rhs = std::get<0>(instr->get_operand(1).operand);
        auto l_node = get_node(lhs);
        auto r_node = get_node(rhs);
        exist_node->second->l = l_node;
        exist_node->second->r = r_node;
    }
}

bool AlgebraicSimplification::is_constant(Value *v) {
    if (dynamic_cast<Constant *>(v))
        return true;
    return false;
}

void AlgebraicSimplification::reassociation(shared_ptr<ExpTreeNode> node) {
    if (node->dealed)
        return;
    if (node->op == alop::leaf) {
        if (!node->is_const)
            node->add_term(node->val, 1);
        return;
    }
    reassociation(node->l);
    reassociation(node->r);
    switch (node->op) {
        case alop::add:
            // add_node(node);
            // break;
        case alop::sub:
            add_sub_node_new(node);
            break;
        case alop::mul:
            mul_node_new(node);
            break;
        case alop::div:
            div_node(node);
            break;

        default:
            break;
    }
    node->dealed = true;
}

void AlgebraicSimplification::add_sub_node(shared_ptr<ExpTreeNode> node) {
    auto lch = node->l;
    auto rch = node->r;
    // if (node->type == ExpTreeNode::ExpType::int_) {
    // 常数表达式计算
    // if (lch->is_const && node->r->is_const) {
    //     node->is_const = true;
    //     node->const_val = lch->get_const_int_val() + node->r->get_const_int_val();
    //     node->new_val = ConstantInt::get(std::get<int>(node->const_val), m_);
    //     return;
    // }

    /*
    c:constant values,t:non-constant values
            +
          /   \
         +     +
        / \   / \
       t1 c1 c2  t2
             |
             |
             V
             +
          /     \
        c1+c2  t1+t2

            +
          /   \
         +     +
        / \   / \
       c1 t1 t2  t3
            |
            |
            V
            +
           / \
         c1   t1+t2+t3
    */

    std::vector<shared_ptr<ExpTreeNode>> constant_list;
    std::vector<shared_ptr<ExpTreeNode>> negtive_const_list;
    std::vector<shared_ptr<ExpTreeNode>> non_const_list;
    std::vector<shared_ptr<ExpTreeNode>> negtive_non_con_list;
    // 是否需要重排
    bool reass = false;
    bool new_val = true;
    if (lch->is_const)
        constant_list.push_back(lch);
    else {
        if (lch->op == alop::add) {
            if (lch->l->is_const) {
                constant_list.push_back(lch->l);
                reass = true;
                non_const_list.push_back(lch->r);
            } else
                non_const_list.push_back(lch);

        } else if (lch->op == alop::sub) {
            if (lch->l->is_const) {
                constant_list.push_back(lch->l);
                reass = true;
                negtive_non_con_list.push_back(lch->r);
            } else
                non_const_list.push_back(lch);
        } else
            non_const_list.push_back(lch);
    }
    if (rch->is_const) {
        if (node->op == alop::add)
            constant_list.push_back(rch);
        else
            negtive_const_list.push_back(rch);
        reass = true;
    } else {
        if (rch->op == alop::add) {
            // t1 +/- (t2 + t3)
            if (rch->l->is_const) {
                if (node->op == alop::add) {
                    // t1 + (c + t2)
                    constant_list.push_back(rch->l);
                    non_const_list.push_back(rch->r);
                } else {
                    // t1 - (c + t2)
                    negtive_const_list.push_back(rch->l);
                    negtive_non_con_list.push_back(rch->r);
                }
                reass = true;
            } else if (node->op == alop::add)
                non_const_list.push_back(rch);
            else
                negtive_non_con_list.push_back(rch);

        } else if (rch->op == alop::sub) {
            // t1 +/- (t2 - t3)
            if (rch->l->is_const) {
                if (node->op == alop::add) {
                    // t1 + (c - t2)
                    constant_list.push_back(rch->l);
                    negtive_non_con_list.push_back(rch->r);
                } else {
                    // t1 - (c - t2)
                    negtive_const_list.push_back(rch->l);
                    non_const_list.push_back(rch->r);
                }
                reass = true;
            } else if (node->op == alop::add)
                non_const_list.push_back(rch);
            else
                negtive_non_con_list.push_back(rch);

        } else if (node->op == alop::add)
            non_const_list.push_back(rch);
        else
            negtive_non_con_list.push_back(rch);
    }
    if (!reass)
        return;
    shared_ptr<ExpTreeNode> new_lch, new_rch;
    alop new_val_op = alop::add;
    // if (constant_list.size() == 1 && )
    //     // 仅有一个常数项，此时不需要创建新的结点
    //     /*
    //           /\      or      /|
    //         t  c+t        t+c  t+t

    //         etc.
    //     */
    //     new_lch = constant_list.size() == 1 ? constant_list[0] : negtive_const_list[0];
    // else {
    if (node->type == ExpTreeNode::ExpType::int_) {
        int new_const_val = 0;
        for (auto c : constant_list)
            new_const_val += c->get_const_int_val();
        for (auto nc : negtive_const_list)
            new_const_val -= nc->get_const_int_val();
        if (non_const_list.size() + negtive_non_con_list.size() == 0) {
            // 两个操作数均为常数，该结点成为新的常数结点
            /*
              /\
             c  c
            */
            if (constant_list.size() + negtive_const_list.size() != 2) {
                exit(214);
                LOG_ERROR << "non-constant node with 2 constant child nodes";
            }
            node->is_const = true;
            node->set_const_val(new_const_val);
            node->set_new_val(ConstantInt::get(new_const_val, m_));
            return;
        }
        new_lch = std::shared_ptr<ExpTreeNode>(
            new ExpTreeNode(ConstantInt::get(new_const_val, m_), nullptr, nullptr, alop::leaf));
        new_lch->new_val = new_lch->val;
    } else {
        float new_const_val = 0;
        for (auto c : constant_list)
            new_const_val += c->get_const_float_val();
        for (auto nc : negtive_const_list)
            new_const_val -= nc->get_const_float_val();
        if (non_const_list.size() + negtive_non_con_list.size() == 0) {
            if (constant_list.size() + negtive_const_list.size() != 2) {
                exit(215);
                LOG_ERROR << "non-constant node with 2 constant child nodes";
            }
            node->is_const = true;
            node->set_const_val(new_const_val);
            node->set_new_val(ConstantFP::get(new_const_val, m_));
            return;
        }
        new_lch = std::shared_ptr<ExpTreeNode>(
            new ExpTreeNode(ConstantFP::get(new_const_val, m_), nullptr, nullptr, alop::leaf));
        new_lch->new_val = new_lch->val;
    }
    // }
    if (constant_list.size() + negtive_const_list.size() == 1 && rch->is_const) {
        // 仅需交换两个操作数的情况
        /*
            /  \
         t/t+t  c
        */
        new_rch = lch;
        new_val_op = alop::add;
        if (node->op == alop::add)
            new_val = false;
    } else if (non_const_list.size() + negtive_non_con_list.size() == 1) {  // 仅有一个非常数项的情况
        new_rch = non_const_list.size() == 1 ? non_const_list[0] : negtive_non_con_list[0];
        new_val_op = non_const_list.size() == 1 ? alop::add : alop::sub;
    } else {
        // new_rch = virtual_non_const_node;
        if (non_const_list.size() + negtive_non_con_list.size() != 2) {
            exit(216);
            LOG_ERROR << "number of non-constant node error";
            exit(ERROR_IN_ALGE_SIMP);
        }
        std::shared_ptr<Instruction> new_instr;
        if (non_const_list.size() == 2) {
            new_instr =
                BinaryInst::create(non_const_list[0]->get_newest_val()->get_type(),
                                   node->type == ExpTreeNode::ExpType::int_ ? Instruction::add : Instruction::fadd,
                                   non_const_list[0]->get_newest_val(),
                                   non_const_list[1]->get_newest_val(),
                                   nullptr);
        } else if (negtive_non_con_list.size() == 2) {
            new_instr =
                BinaryInst::create(negtive_non_con_list[0]->get_newest_val()->get_type(),
                                   node->type == ExpTreeNode::ExpType::int_ ? Instruction::add : Instruction::fadd,
                                   negtive_non_con_list[0]->get_newest_val(),
                                   negtive_non_con_list[1]->get_newest_val(),
                                   nullptr);
            new_val_op = alop::sub;
        } else {
            new_instr =
                BinaryInst::create(negtive_non_con_list[0]->get_newest_val()->get_type(),
                                   node->type == ExpTreeNode::ExpType::int_ ? Instruction::sub : Instruction::fsub,
                                   non_const_list[0]->get_newest_val(),
                                   negtive_non_con_list[0]->get_newest_val(),
                                   nullptr);
        }
        // auto new_instr_lop = non_const_list[0]->get_newest_val();
        // for (int i = 1; i < non_const_list.size(); i++) {
        //     auto new_instr =
        //         BinaryInst::create(new_instr_lop->get_type(),
        //                            node->type == ExpTreeNode::ExpType::int_ ? Instruction::add : Instruction::fadd,
        //                            new_instr_lop,
        //                            non_const_list[i]->get_newest_val(),
        //                            nullptr);
        //     // std::static_pointer_cast<Instruction>(new_instr_lop)->get_parent()->get_instructions().pop_back();
        //     node->new_instrs.push_back(new_instr);
        //     new_instr_lop = new_instr;
        // }
        node->new_instrs.push_back(new_instr);
        new_rch = shared_ptr<ExpTreeNode>(new ExpTreeNode(new_instr, nullptr, nullptr, alop::leaf));
        new_rch->set_new_val(new_instr);
        new_rch->is_const = false;
    }
    node->l = new_lch;
    node->r = new_rch;
    if (new_val) {
        Instruction::OpID ins_op;
        if (new_val_op == alop::add)
            ins_op = node->type == ExpTreeNode::ExpType::int_ ? Instruction::add : Instruction::fadd;
        else
            ins_op = node->type == ExpTreeNode::ExpType::int_ ? Instruction::sub : Instruction::sub;
        node->new_val = BinaryInst::create(
            node->val->get_type(), ins_op, node->l->get_newest_val(), node->r->get_newest_val(), nullptr);
        node->op = new_val_op;
    }

    // } else {
    //     if (node->l->is_const && node->r->is_const) {
    //         node->is_const = true;
    //         node->const_val = node->l->get_const_float_val() + node->r->get_const_float_val();
    //         node->new_val = ConstantFP::get(std::get<float>(node->const_val), m_);
    //         return;
    //     }
    // }
    // if (node->l->is_const && node->r->is_const) {
    //     node->is_const = true;
    //     if (node->type == ExpTreeNode::ExpType::int_) {
    //         node->const_val = node->l->get_const_int_val() + node->r->get_const_int_val();
    //         node->new_val = ConstantInt::get(std::get<int>(node->const_val), m_);
    //     } else {
    //         node->const_val = node->l->get_const_float_val() + node->r->get_const_float_val();
    //         node->new_val = ConstantFP::get(std::get<float>(node->const_val), m_);
    //     }
    //     return;
    // }
}
void AlgebraicSimplification::add_sub_node_new(shared_ptr<ExpTreeNode> node) {
    if (node->type == ExpTreeNode::ExpType::int_) {
        if (node->op == alop::add) {
            node->add_term(node->l, 1);
            node->add_term(node->r, 1);
        } else if (node->op == alop::sub) {
            node->add_term(node->l, 1);
            node->add_term(node->r, -1);
        }
    }
    if (node->terms.empty()) {
        node->is_const = true;
    }
}

void AlgebraicSimplification::mul_node_new(shared_ptr<ExpTreeNode> node) {
    if (node->type == ExpTreeNode::ExpType::int_) {
        if (node->l->is_const && node->r->is_const) {
            node->const_val = node->l->get_const_int_val() * node->r->get_const_int_val();
            node->is_const = true;
            return;
        }
        if (node->l->is_const || node->r->is_const) {
            shared_ptr<ExpTreeNode> c_node;
            shared_ptr<ExpTreeNode> nc_node;
            if (node->l->is_const) {
                c_node = node->l;
                nc_node = node->r;
            } else {
                c_node = node->r;
                nc_node = node->l;
            }
            node->add_term(nc_node, c_node->get_const_int_val());
            node->is_const = false;
            return;
        }
        node->add_term(node->val, 1);
    }
}

void AlgebraicSimplification::mul_node(shared_ptr<ExpTreeNode> node) {
    auto lch = node->l;
    auto rch = node->r;
    if (node->l->is_const && node->r->is_const) {
        node->is_const = true;
        if (node->type == ExpTreeNode::ExpType::int_) {
            node->const_val = node->l->get_const_int_val() * node->r->get_const_int_val();
            node->new_val = ConstantInt::get(std::get<int>(node->const_val), m_);
        } else {
            node->const_val = node->l->get_const_float_val() * node->r->get_const_float_val();
            node->new_val = ConstantFP::get(std::get<float>(node->const_val), m_);
        }
        return;
    }
    if (rch->is_const) {
        std::swap(node->l, node->r);
        std::swap(lch, rch);
    }
    if (lch->op == alop::leaf && rch->op == alop::leaf) {
        // if (rch->is_const) {
        //     std::swap(lch, rch);
        // }
        return;
    }
    if (rch->op == alop::leaf && lch->is_const) {
        // std::swap(lch, rch);
        return;
    }

    // (c1 +/- t) * c2 -> c1*c1 +/- c2*t
    shared_ptr<ExpTreeNode> new_lch, new_rch;
    if ((rch->op == alop::add || rch->op == alop::sub) && lch->is_const) {
        shared_ptr<ExpTreeNode> addi_node = rch;
        shared_ptr<ExpTreeNode> con_node = lch;
        if (addi_node->l->is_const) {
            if (node->type == ExpTreeNode::ExpType::int_) {
                int const_val = addi_node->l->get_const_int_val() * con_node->get_const_int_val();
                new_lch = std::shared_ptr<ExpTreeNode>(
                    new ExpTreeNode(ConstantInt::get(const_val, m_), nullptr, nullptr, alop::leaf));

            } else {
                float const_val = addi_node->l->get_const_float_val() * con_node->get_const_float_val();
                new_lch = std::shared_ptr<ExpTreeNode>(
                    new ExpTreeNode(ConstantFP::get(const_val, m_), nullptr, nullptr, alop::leaf));
            }
            auto new_r_instr =
                BinaryInst::create(node->val->get_type(),
                                   node->type == ExpTreeNode::ExpType::int_ ? Instruction::mul : Instruction::fmul,
                                   con_node->get_newest_val(),
                                   addi_node->r->get_newest_val(),
                                   nullptr);
            node->new_instrs.push_back(new_r_instr);
            new_rch = shared_ptr<ExpTreeNode>(new ExpTreeNode(new_r_instr, nullptr, nullptr, alop::leaf));
            new_rch->set_new_val(new_r_instr);
            Instruction::OpID ins_op;
            if (addi_node->op == alop::add)
                ins_op = node->type == ExpTreeNode::ExpType::int_ ? Instruction::add : Instruction::fadd;
            else
                ins_op = node->type == ExpTreeNode::ExpType::int_ ? Instruction::sub : Instruction::sub;
            node->set_new_val(BinaryInst::create(
                node->val->get_type(), ins_op, new_lch->get_newest_val(), new_rch->get_newest_val(), nullptr));
            node->op = addi_node->op;
            node->l = new_lch;
            node->r = new_rch;
        }
        return;
    }
    if ((lch->op == alop::leaf || lch->op == alop::mul || lch->is_const) &&
        (rch->op == alop::leaf || rch->op == alop::mul)) {
        if (node->type == ExpTreeNode::ExpType::float_)
            return;
        std::vector<shared_ptr<ExpTreeNode>> constant_list;
        std::vector<shared_ptr<ExpTreeNode>> non_const_list;
        bool reass = false;
        if (lch->is_const) {
            constant_list.push_back(lch);
        } else if (lch->op == alop::mul && lch->l->is_const) {
            constant_list.push_back(lch->l);
            non_const_list.push_back(lch->r);
            reass = true;
        } else
            non_const_list.push_back(lch);
        if (rch->op == alop::mul && rch->l->is_const) {
            constant_list.push_back(rch->l);
            non_const_list.push_back(rch->r);
            reass = true;
        } else
            non_const_list.push_back(rch);
        if (!reass)
            return;
        if (constant_list.empty() || constant_list.size() > 2) {
            exit(217);
            LOG_ERROR << "error case, size of constant list is " << constant_list.size();
            exit(ERROR_IN_ALGE_SIMP);
        }
        if (constant_list.size() == 1)
            new_lch = constant_list[0];
        else if (constant_list.size() == 2) {
            if (node->type == ExpTreeNode::ExpType::int_) {
                int const_num = constant_list[0]->get_const_int_val() * constant_list[1]->get_const_int_val();
                new_lch = shared_ptr<ExpTreeNode>(
                    new ExpTreeNode(ConstantInt::get(const_num, m_), nullptr, nullptr, alop::leaf));
            } else {
                float const_num = constant_list[0]->get_const_float_val() * constant_list[1]->get_const_float_val();
                new_lch = shared_ptr<ExpTreeNode>(
                    new ExpTreeNode(ConstantFP::get(const_num, m_), nullptr, nullptr, alop::leaf));
            }
        }
        if (!(non_const_list.size() == 1 or non_const_list.size() == 2)) {
            exit(218);
            LOG_ERROR << "error case, size of non-constant list is " << constant_list.size();
            exit(ERROR_IN_ALGE_SIMP);
        }
        if (non_const_list.size() == 1)
            new_rch = non_const_list[0];
        else {
            shared_ptr<Instruction> new_instr =
                BinaryInst::create(node->val->get_type(),
                                   node->type == ExpTreeNode::ExpType::int_ ? Instruction::mul : Instruction::fmul,
                                   non_const_list[0]->get_newest_val(),
                                   non_const_list[1]->get_newest_val(),
                                   nullptr);
            node->new_instrs.push_back(new_instr);
            new_rch = std::shared_ptr<ExpTreeNode>(new ExpTreeNode(new_instr, nullptr, nullptr, alop::leaf));
            // new_rch->set_new_val()
        }
        node->new_val =
            BinaryInst::create(node->val->get_type(),
                               node->type == ExpTreeNode::ExpType::int_ ? Instruction::mul : Instruction::fmul,
                               new_lch->get_newest_val(),
                               new_rch->get_newest_val(),
                               nullptr);
        node->l = new_lch;
        node->r = new_rch;
        return;
    }
}
void AlgebraicSimplification::div_node(shared_ptr<ExpTreeNode> node) {
    if (node->l->is_const && node->r->is_const) {
        node->is_const = true;
        if (node->type == ExpTreeNode::ExpType::int_) {
            node->const_val = node->l->get_const_int_val() / node->r->get_const_int_val();
            node->new_val = ConstantInt::get(std::get<int>(node->const_val), m_);
        } else {
            node->const_val = node->l->get_const_float_val() / node->r->get_const_float_val();
            node->new_val = ConstantFP::get(std::get<float>(node->const_val), m_);
        }
        return;
    }
    node->add_term(node->val, 1);
    // if (node->r->is_const) {
    //     if (node->type == ExpTreeNode::ExpType::float_) {
    //         if (float r_f = node->r->get_const_float_val(); test_reciprocal_exception(r_f)) {
    //             auto node_instr = std::static_pointer_cast<Instruction>(node->val);
    //             node->new_val =
    //                 BinaryInst::create_fmul(node->l->val, ConstantFP::get(1 / r_f, m_), node_instr->get_parent());
    //             node_instr->get_parent()->get_instructions().pop_back();
    //         }
    //     }
    // }
}

bool test_reciprocal_exception(float c) {
    std::feclearexcept(FE_ALL_EXCEPT);
    volatile float _ = 1 / c;
    if (std::fetestexcept(FE_ALL_EXCEPT))
        return false;
    return true;
}

void AlgebraicSimplification::int_mul_weaken(std::shared_ptr<Function> func) {
    for (auto bb : func->get_basic_blocks()) {
        for (auto instr_itr = bb->get_instructions().begin(); instr_itr != bb->get_instructions().end(); instr_itr++) {
            auto instr = *instr_itr;
            if (instr->is_mul() &&
                (is_constant(instr->get_operand(0).get()) || is_constant(instr->get_operand(1).get()))) {
                int const_value;
                Value *non_const_op;
                if (is_constant(instr->get_operand(0).get())) {
                    const_value = static_cast<ConstantInt *>(instr->get_operand(0).get())->get_value();
                    non_const_op = instr->get_operand(1).get();
                } else {
                    const_value = static_cast<ConstantInt *>(instr->get_operand(1).get())->get_value();
                    non_const_op = instr->get_operand(0).get();
                }
                if (const_value <= 0)
                    continue;
                for (int sub : {0, -1, 1}) {
                    int const_value_ = const_value - sub;
                    if ((const_value_ & (const_value_ - 1)) == 0) {  // if const_value is power of 2
                        int pow = 0;
                        shared_ptr<Value> new_val;
                        for (; const_value_ >> (pow + 1); pow++)
                            ;
                        if (pow == 0)
                            new_val = non_const_op->shared_from_this();
                        else {
                            auto new_shl_inst = BinaryInst::create(m_->get_int32_type(),
                                                                   Instruction::shl,
                                                                   non_const_op->shared_from_this(),
                                                                   ConstantInt::get(pow, m_),
                                                                   nullptr);
                            bb->get_instructions().insert(instr_itr, new_shl_inst);
                            new_shl_inst->set_parent(bb.get());
                            if (sub == 1) {
                                auto new_add_inst = BinaryInst::create(m_->get_int32_type(),
                                                                       Instruction::add,
                                                                       new_shl_inst,
                                                                       non_const_op->shared_from_this(),
                                                                       nullptr);
                                new_add_inst->set_parent(bb.get());
                                new_val = new_add_inst;
                                bb->get_instructions().insert(instr_itr, new_add_inst);
                            } else if (sub == -1) {
                                auto new_add_inst = BinaryInst::create(m_->get_int32_type(),
                                                                       Instruction::sub,
                                                                       new_shl_inst,
                                                                       non_const_op->shared_from_this(),
                                                                       nullptr);
                                new_add_inst->set_parent(bb.get());
                                new_val = new_add_inst;
                                bb->get_instructions().insert(instr_itr, new_add_inst);
                            } else
                                new_val = new_shl_inst;
                        }
                        instr->replace_all_use_with(new_val);
                        instr->get_use_list().clear();
                        break;
                    }
                }
            }
        }
    }
}

void AlgebraicSimplification::eliminate_invalid_operation(shared_ptr<Function> func) {
    for (auto bb : func->get_basic_blocks()) {
        for (auto instr_itr = bb->begin(); instr_itr != bb->end();) {
            auto instr = *instr_itr;
            std::shared_ptr<Value> new_val = nullptr;
            if (instr->get_type()->is_integer_type()) {
                if (instr->isBinary()) {
                    auto op1 = instr->get_operand(0);
                    auto op2 = instr->get_operand(1);
                    auto c1 = utils::get_const_int_val(op1.get());
                    auto c2 = utils::get_const_int_val(op2.get());
                    if (instr->is_add()) {
                        if (c1 == 0)
                            new_val = op2->shared_from_this();
                        else if (c2 == 0)
                            new_val = op1->shared_from_this();
                    } else if (instr->is_sub()) {
                        if (c2 == 0)
                            new_val = op1->shared_from_this();
                    } else if (instr->is_mul()) {
                        if (c1 == 1)
                            new_val = op2->shared_from_this();
                        else if (c2 == 1)
                            new_val = op1->shared_from_this();
                        else if (c1 == 0 || c2 == 0) {
                            new_val = ConstantInt::get(0, m_);
                        }
                    } else if (instr->is_div()) {
                        if (c2 == 1)
                            new_val = op1->shared_from_this();
                        else if (c1 == 0)
                            new_val = ConstantInt::get(0, m_);
                    } else if (instr->is_ashr() || instr->is_and_() || instr->is_lshr() || instr->is_shl()) {
                        if (c2 == 0)
                            new_val = op1->shared_from_this();
                    }
                }
            }
            if (new_val != nullptr) {
                instr->replace_all_use_with(new_val);
                instr_itr = bb->get_instructions().erase(instr_itr);
            } else
                instr_itr++;
        }
    }
}

#include "Peephole.hh"

#include "AliasAnalysis.hh"
#include "Instruction.h"
#include "utils.hh"

#include <map>
void Peephole::run() {
    for (auto f : m_->get_functions())
        for (auto bb : f->get_basic_blocks())
            // run_peephole(bb.get());
            run_peephole_aa(bb.get());
}
// rewrite peephole using alias analysis
void Peephole::run_peephole_aa(BasicBlock *bb) {
    AliasAnalysis aa;
    std::map<Value *, StoreInst *> ptr2store;  // ptr2store update of `ptr` in a basic block
    std::map<Value *, Value *> ptr2val;        // ptr2store update of `ptr` in a basic block
    for (auto it = bb->begin(); it != bb->end(); ++it) {
        auto i = it->get();
        if (i->is_call()) {
            ptr2store.clear();
            ptr2val.clear();
        }  // TODO: check pure funcs
        else if (i->is_store()) {
            auto rval = dynamic_cast<StoreInst *>(i)->get_rval().get();
            auto lval = dynamic_cast<StoreInst *>(i)->get_lval().get();
            if (std::none_of(ptr2store.begin(), ptr2store.end(), [&](auto tup) {
                    auto v = tup.first;
                    auto res = aa.alias(lval, v);
                    return res == AliasResult::MayAlias or res == AliasResult::MustAlias;
                })) {  // none of the ptr might ever alias with `lval`, insert to map
                ptr2store.insert({lval, dynamic_cast<StoreInst *>(i)});
                ptr2val.insert({lval, rval});
            } else if (std::any_of(ptr2store.begin(), ptr2store.end(), [&](auto tup) {
                           auto v = tup.first;
                           auto res = aa.alias(lval, v);
                           return res == AliasResult::MayAlias;
                       })) {
                // might corrupt all the values, clear `ptr2store`
                ptr2store.clear();
                ptr2val.clear();
                ptr2store.insert({lval, dynamic_cast<StoreInst *>(i)});
                ptr2val.insert({lval, rval});
            } else {
                // we assure values in `ptr2store` are noalias (distinct)
                auto map_iter = std::find_if(ptr2store.begin(), ptr2store.end(), [&](auto tup) {
                    auto v = tup.first;
                    auto res = aa.alias(lval, v);
                    return res == AliasResult::MustAlias;
                });

                auto map_iter_val = std::find_if(ptr2val.begin(), ptr2val.end(), [&](auto tup) {
                    auto v = tup.first;
                    auto res = aa.alias(lval, v);
                    return res == AliasResult::MustAlias;
                });
                if (map_iter == ptr2store.end()) {
                    ptr2store.insert({lval, dynamic_cast<StoreInst *>(i)});
                    ptr2val.insert({lval, rval});
                } else {
                    auto store = map_iter->second;
                    bb->delete_instr(store->get_iterator());
                    ptr2store.erase(map_iter);
                    ptr2val.erase(map_iter_val);
                    ptr2store.insert({lval, dynamic_cast<StoreInst *>(i)});
                    ptr2val.insert({lval, rval});
                }
            }
        } else if (i->is_load()) {
            auto lval = i->get_operand(0).get();
            auto map_iter = std::find_if(ptr2val.begin(), ptr2val.end(), [&](auto tup) {
                auto v = tup.first;
                auto res = aa.alias(lval, v);
                return res == AliasResult::MustAlias;
            });
            if (map_iter == ptr2val.end()) {
                ptr2val[lval] = i;
            } else
                i->replace_all_use_with(map_iter->second->shared_from_this());
        }
    }
}

void Peephole::run_peephole(BasicBlock *bb) {
    std::map<Value *, StoreInst *> ptr2store;  // ptr2store update of `ptr` in a basic block
    for (auto it = bb->begin(); it != bb->end(); ++it) {
        auto i = it->get();
        Value *ptr, *rval;
        if (i->is_call()) {
            ptr2store.clear();
        } else if (i->is_store()) {
            auto store = dynamic_cast<StoreInst *>(i);
            ptr = store->get_lval().get();
            rval = store->get_rval().get();
            auto old_store = ptr2store[ptr];
            if (old_store)
                bb->delete_instr(old_store->get_iterator());
            ptr2store[ptr] = store;
        } else if (i->is_load()) {
            ptr = i->get_operand(0).get();
            auto current = ptr2store[ptr] ? ptr2store[ptr]->get_rval() : nullptr;
            if (current)
                i->replace_all_use_with(current);
        } else if (Value * v; match(i, m_ptrtoint(m_inttoptr(m_value(v)))) or
                              match(i, m_inttoptr(m_ptrtoint(m_value(v)))) and i->get_type() == v->get_type() and
                                  !dynamic_cast<GlobalVariable *>(v)) {
            i->replace_all_use_with(v->shared_from_this());
        }
    }
}
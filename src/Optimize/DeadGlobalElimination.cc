#include "AliasAnalysis.hh"
#include "DeadGlobalElim.hh"
std::list<Instruction *> removes;
std::list<Instruction *> replaces;
template <typename T>
bool is_gep_or(Use u) {
    auto store = dynamic_cast<T *>(u.val_);
    if (store) {
        if (dynamic_cast<LoadInst *>(store))
            replaces.push_back(store);

        removes.push_back(store);
        return true;
    }
    auto GEP = dynamic_cast<GetElementPtrInst *>(u.val_);
    if (GEP) {
        removes.push_back(GEP);
        auto gep_use = GEP->get_use_list();
        return std::all_of(gep_use.begin(), gep_use.end(), is_gep_or<T>);
    }
    return false;
}

void DeadGlobalElimination::run() {
    auto &gl_list = m_->get_global_variables();
    for (auto glit = gl_list.begin(); glit != gl_list.end();) {
        removes.clear();
        replaces.clear();
        auto gl = *glit;
        auto &gl_use = gl->get_use_list();
        if (std::all_of(gl_use.begin(), gl_use.end(), is_gep_or<StoreInst>)) {
            std::for_each(removes.begin(), removes.end(), [](Instruction *i) {
                i->get_parent()->delete_instr(std::dynamic_pointer_cast<Instruction>(i->shared_from_this()));
            });
            glit = gl_list.erase(glit);
            continue;
        }
        removes.clear();
        replaces.clear();
        if (std::dynamic_pointer_cast<ConstantZero>(gl->get_init()) and
            std::all_of(gl_use.begin(), gl_use.end(), is_gep_or<LoadInst>)) {
            std::for_each(replaces.begin(), replaces.end(), [this](Instruction *i) {
                if (dynamic_cast<LoadInst *>(i))
                    i->replace_all_use_with(ConstantInt::get(0, m_));
            });
            std::for_each(removes.begin(), removes.end(), [](Instruction *i) {
                i->get_parent()->delete_instr(std::dynamic_pointer_cast<Instruction>(i->shared_from_this()));
            });
            glit = gl_list.erase(glit);
            continue;
        }
        removes.clear();
        replaces.clear();
        if (std::all_of(gl_use.begin(), gl_use.end(), is_gep_or<LoadInst>)) {
            std::for_each(replaces.begin(), replaces.end(), [&, this](Instruction *i) {
                if (not isa<ConstantArray>(gl->get_init()))
                    return;
                auto gl_arr = std::dynamic_pointer_cast<ConstantArray>(gl->get_init());
                if (not isa<ConstantInt>(gl_arr->get_element_value(0)))
                    return;
                if (dynamic_cast<LoadInst *>(i)) {
                    auto ptr = i->get_operand(0).get();
                    MemAddress addr = ptr;
                    if (addr.get_const_offset().has_value()) {
                        auto offset = addr.get_const_offset().value() /4;
                        i->replace_all_use_with(gl_arr->get_element_value(offset));
                    }
                }
            });
        }

        glit++;
    }
}

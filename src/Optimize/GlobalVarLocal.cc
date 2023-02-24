#include "GlobalVarLocal.hh"

#include "Mem2Reg.hh"
#include "SortBB.hh"
#include "queue"

void GloVarLocal::run() {
    remove_uncalled_func();
    SortBB{m_}.run();
    bool localized = false;
    for (auto glo : m_->get_global_variables()) {
        std::unordered_set<Function *> use_functions;
        for (auto glo_use : glo->get_use_list()) {
            auto use_inst = dynamic_cast<Instruction *>(glo_use.val_);
            if (!use_inst) {
                LOG_ERROR << "global variable " << glo->print() << " has an no-Inst user";
                exit_if(true, ERROR_IN_GLOBAL_VAR_LOCAL_GLO_USER_ERROR);
            }
            auto use_func = use_inst->get_function();
            if (!use_func->get_use_list().empty() || use_func->get_name() == "main") {
                use_functions.insert(use_func);
            }
        }
        if (use_functions.size() != 1) {
            LOG_DEBUG << "global var " << glo->print() << " cannot be localized";
            continue;
        }
        auto use_func = *use_functions.begin();
        auto entry = use_func->get_entry_block();
        auto glo_type = glo->get_type()->get_pointer_element_type();
        if (glo_type->is_array_type()) {
            // TODO: 数组局部化
            continue;
        } else {
            auto entry_begin = entry->begin();
            auto new_alloca = AllocaInst::create(glo_type, &entry_begin);
            std::shared_ptr<Constant> init;
            if (glo->get_init()->is_constant_zero()) {
                if (glo->get_init()->get_type()->is_integer_type())
                    init = ConstantInt::get(0, m_);
                else
                    init = ConstantFP::get(0.0, m_);

            } else
                init = glo->get_init();
            StoreInst::create(init, new_alloca, &entry_begin);
            glo->replace_all_use_with(new_alloca);
        }
        LOG_INFO << "global var " << glo->print() << " has been localized";
        localized = true;
    }
    if (localized) {
        Mem2Reg m2r(m_);
        m2r.run();
    }
}

void GloVarLocal::remove_uncalled_func() {
    auto &funcs = m_->get_functions();
    for (auto func_it = funcs.begin(); func_it != funcs.end();) {
        if ((*func_it)->get_use_list().empty() && (*func_it)->get_name() != "main") {
            for (auto bb : (*func_it)->get_basic_blocks()) {
                for (auto inst : bb->get_instructions()) {
                    inst->remove_use_of_ops();
                }
            }
            func_it = funcs.erase(func_it);
        } else
            func_it++;
    }
}

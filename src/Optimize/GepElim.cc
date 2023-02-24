#include "GepElim.hh"
#include "SortBB.hh"

#include "utils.hh"

void GepElimination::run() {
    SortBB{m_}.run();
    for (auto f : m_->get_functions()) {
        if (f->is_declaration())
            continue;
        addr.clear();
        entry = f->get_entry_block();
        for (auto gl : m_->get_global_variables()) {
            if (static_cast<PointerType *>(gl->get_type())->get_pointer_element_type()->is_array_type())
                continue;
            auto ptr2int = PtrToIntInst::create_ptrtoint(gl);
            auto int2ptr = IntToPtrInst::create_inttoptr(ptr2int, gl->get_type());
            entry->add_instr_begin(int2ptr);
            entry->add_instr_begin(ptr2int);
            ptr2int->set_parent(entry.get());
            int2ptr->set_parent(entry.get());
            gl->replace_use_with_when(int2ptr, [&](User *v) {
                Instruction *i = dynamic_cast<Instruction *>(v);
                return i and i != ptr2int.get() and i->get_parent()->get_parent() == f.get();
            });
        }
        for (auto bb : f->get_basic_blocks())
            remove_gep(bb.get());
    }
}

void GepElimination::remove_gep(BasicBlock *bb) {
    auto &insts = bb->get_instructions();
    for (auto it = insts.begin(); it != insts.end();) {  // ++it at loop end
        GetElementPtrInst *GEP = dynamic_cast<GetElementPtrInst *>(it->get());
        if (not GEP) {
            it++;
            continue;
        }
        // base pointer of gep instruction
        auto op = dynamic_cast<Instruction *>(GEP->get_operand(0).get());
        auto gl = dynamic_cast<GlobalVariable *>(GEP->get_operand(0).get());
        auto arg = dynamic_cast<Argument *>(GEP->get_operand(0).get());
        Value *baseptr{op};
        std::shared_ptr<Value> ptr_as_int{nullptr};
        if (gl) {
            std::shared_ptr<Value> ptr2int;
            if (not addr.count(gl)) {
                auto ptr2int = PtrToIntInst::create_ptrtoint(gl->shared_from_this());
                entry->add_instr_begin(ptr2int);
                ptr2int->set_parent(entry.get());
                addr[gl] = ptr2int;
            }
            baseptr = gl;
        } else if (arg) {
            if (not addr.count(arg)) {
                auto ptr2int = PtrToIntInst::create_ptrtoint(arg->shared_from_this());
                entry->add_instr_begin(ptr2int);
                ptr2int->set_parent(entry.get());
                addr[arg] = ptr2int;
            }
            baseptr = arg;
        } else if (op->is_phi()) {
            // LOG_DEBUG << "PHI";
            std::shared_ptr<Value> ptr2int;
            if (not(ptr2int = addr[op])) {
                ptr2int = PtrToIntInst::create_ptrtoint(op->shared_from_this(), &it);
                addr[op] = ptr2int;
            }
        } else if (op->is_alloca()) {
            auto it = ++op->get_iterator();
            std::shared_ptr<Value> ptr2int;
            if (not(ptr2int = addr[op])) {
                ptr2int = PtrToIntInst::create_ptrtoint(op->shared_from_this(), &it);
                addr[op] = ptr2int;
            }
        }
        auto target_ptr_ty = m_->get_ptrtoint_type();
        Type *element_type = baseptr->get_type();
        // LOG_DEBUG << baseptr->print();
        std::shared_ptr<Value> prev = addr.at(baseptr);
        for (auto i = 1u; i < GEP->get_num_operand(); i++) {
            if (element_type->is_array_type()) {
                // LOG_DEBUG << "array";
                element_type = static_cast<ArrayType *>(element_type)->get_element_type();
            } else if (element_type->is_pointer_type()) {
                // LOG_DEBUG << "pointer";
                element_type = static_cast<PointerType *>(element_type)->get_element_type();
            } else {
                exit(221);
                LOG_ERROR << "Unknown element type";
            }
            auto index = GEP->get_operand(i)->shared_from_this();
            if (auto ci = std::dynamic_pointer_cast<ConstantInt>(index); ci and ci->get_value() == 0)
                continue;
            if (index->get_type() != target_ptr_ty)  // convert to i64
                index = ZextInst::create_zext(index, target_ptr_ty, &it);
            auto c = (target_ptr_ty == m_->get_int64_type()) ? ConstantInt::get_i64(element_type->get_size(), m_)
                                                             : ConstantInt::get(element_type->get_size(), m_);
            auto mul_by_size = BinaryInst::create_mul(index, c, &it);
            prev = BinaryInst::create_add(prev, mul_by_size, &it);
        }
        ptr_as_int = prev;
        auto i2p = IntToPtrInst::create_inttoptr(ptr_as_int, GEP->get_type(), &it);
        GEP->replace_all_use_with(i2p);

        addr[i2p.get()] = ptr_as_int;
        it = bb->delete_instr(it);
        // it = insts.erase(it);
    }
}

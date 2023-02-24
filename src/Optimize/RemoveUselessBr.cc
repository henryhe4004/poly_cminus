#include "RemoveUselessBr.hh"

using std::shared_ptr;

// remove_inst_after_br_or_ret
void RemoveUselessBr::run() {
    for (auto func_share : m_->get_functions()) {
        for (auto bb_share : func_share->get_basic_blocks()) {
            bool end = false;
            vector<shared_ptr<Instruction>> to_delete;
            for (auto inst_share : bb_share->get_instructions()) {
                if (end)
                    to_delete.push_back(inst_share);
                if (inst_share->is_br() or inst_share->is_ret())
                    end = true;
            }
            for (auto inst_share : to_delete)
                bb_share->delete_instr(inst_share);
        }

        for (auto bb_share : func_share->get_basic_blocks()) {
            bb_share->clear_pre_basic_blocks();
            bb_share->clear_succ_basic_blocks();
        }

        for (auto bb_share : func_share->get_basic_blocks()) {
            auto last_inst = bb_share->get_instructions().back();
            if (last_inst->is_br()) {
                auto br_inst = last_inst;
                if (br_inst->get_num_operand() == 1) {
                    auto dest = br_inst->get_operand(0).get();
                    auto dest_bb = dynamic_cast<BasicBlock *>(dest);
                    dest_bb->add_pre_basic_block(bb_share.get());
                    bb_share->add_succ_basic_block(dest_bb);
                } else {
                    auto dest1 = br_inst->get_operand(1).get();
                    auto dest2 = br_inst->get_operand(2).get();
                    auto dest1_bb = dynamic_cast<BasicBlock *>(dest1);
                    auto dest2_bb = dynamic_cast<BasicBlock *>(dest2);
                    dest1_bb->add_pre_basic_block(bb_share.get());
                    dest2_bb->add_pre_basic_block(bb_share.get());
                    bb_share->add_succ_basic_block(dest1_bb);
                    bb_share->add_succ_basic_block(dest2_bb);
                }
            }
        }
    }
}

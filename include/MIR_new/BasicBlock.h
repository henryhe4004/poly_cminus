#ifndef SYSYC_BASICBLOCK_H
#define SYSYC_BASICBLOCK_H
#include "Value.h"
#include "logging.hpp"

#include <algorithm>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <list>
#include <memory>
#include <set>
#include <string>

class Function;
class Instruction;
class Module;
class BasicBlock;

// TODO：更换成 hash index，获得 O(1) 的删除时间
using inst_list_type = boost::multi_index_container<
    std::shared_ptr<Instruction>,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::ordered_non_unique<boost::multi_index::identity<std::shared_ptr<Instruction>>>>>;
using bb_list_type = boost::multi_index_container<
    BasicBlock *,
    boost::multi_index::indexed_by<boost::multi_index::sequenced<>,
                                   boost::multi_index::ordered_non_unique<boost::multi_index::identity<BasicBlock *>>>>;
using inst_list_iterator_type = inst_list_type::iterator;
using bb_list_iterator_type = bb_list_type::iterator;

class BasicBlock : public Value {
  public:
    static std::shared_ptr<BasicBlock> create(Module *m, const std::string &name, Function *parent);
    static std::shared_ptr<BasicBlock> create(Module *m,
                                              const std::string &name,
                                              std::list<std::shared_ptr<BasicBlock>>::iterator insert_before);
    // return parent, or null if none.
    Function *get_parent() { return parent_; }

    Module *get_module();

    /****************api about cfg****************/

    std::list<BasicBlock *> &get_pre_basic_blocks() { return pre_bbs_; }
    std::list<BasicBlock *> &get_succ_basic_blocks() { return succ_bbs_; }
    std::list<BasicBlock *> get_pre_basic_blocks_not_ref() { return pre_bbs_; }
    std::list<BasicBlock *> get_succ_basic_blocks_not_ref() { return succ_bbs_; }
    void add_pre_basic_block(BasicBlock *bb) { pre_bbs_.push_back(bb); }
    void add_succ_basic_block(BasicBlock *bb) { succ_bbs_.push_back(bb); }

    void remove_pre_basic_block(BasicBlock *bb);
    void remove_pre_basic_block_phi(BasicBlock *bb);
    void remove_succ_basic_block(BasicBlock *bb) { succ_bbs_.remove(bb); }

    void clear_pre_basic_blocks() { pre_bbs_.clear(); }
    void clear_succ_basic_blocks() { succ_bbs_.clear(); }

    /****************api about cfg****************/

    /// Returns the terminator instruction if the block is well formed or null
    /// if the block is not well formed.
    const std::shared_ptr<Instruction> get_terminator() const;
    std::shared_ptr<Instruction> get_terminator() {
        return std::const_pointer_cast<Instruction>(const_cast<const BasicBlock *>(this)->get_terminator());
    }

    void add_instruction(std::shared_ptr<Instruction> instr);
    void add_instr_begin(std::shared_ptr<Instruction> instr);

    void delete_instr(std::shared_ptr<Instruction> instr);
    inst_list_iterator_type delete_instr(inst_list_iterator_type it);

    bool empty() { return instr_list_.empty(); }
    auto begin() { return instr_list_.begin(); }
    auto end() { return instr_list_.end(); }
    inst_list_iterator_type find_instr(std::shared_ptr<Instruction> instr) {
        auto &sorted_index = instr_list_.get<1>();
        auto it = sorted_index.find(instr);
        return instr_list_.project<0>(it);
    }
    auto insert_before(inst_list_iterator_type pos, std::shared_ptr<Instruction> instr) {
        return instr_list_.insert(pos, instr);
    }
    bool insert_before(std::shared_ptr<Instruction> pos, std::shared_ptr<Instruction> instr) {
        auto itr = find_instr(pos);
        if (itr == end())
            return false;
        insert_before(itr, instr);
        return true;
    }
    auto get_terminator_itr() {
        auto itr = end();
        itr--;
        return itr;
    }
    void insert_before_terminator(std::shared_ptr<Instruction> instr) { insert_before(get_terminator_itr(), instr); }

    int get_num_inst() { return instr_list_.size(); }
    auto &get_instructions() { return instr_list_; }
    std::pair<inst_list_iterator_type, inst_list_iterator_type> get_phis();

    virtual std::string print() override;
    void remove_bb_in_phi(std::shared_ptr<BasicBlock> bb);
    
  private:
    explicit BasicBlock(Module *m, const std::string &name, Function *parent);
    explicit BasicBlock(Module *m,
                        const std::string &name,
                        std::list<std::shared_ptr<BasicBlock>>::iterator insert_before);
    std::list<BasicBlock *> pre_bbs_;
    std::list<BasicBlock *> succ_bbs_;
    inst_list_type instr_list_;
    Function *parent_;
};

#endif  // SYSYC_BASICBLOCK_H

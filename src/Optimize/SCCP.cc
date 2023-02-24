#include "SCCP.hh"

// 由于长度原因放到 .cc 中实现
// 对不同指令的常数值和格值进行计算
// TODO: 在 llvm 的格值求解中，对 load store gep 也有对应的处理，这部分可以加上
// TODO: 还有 cast 类型指令需要补充
SCCPOld::lattice SCCPOld::evaluate_inst(std::shared_ptr<Instruction> inst) {
    LOG_INFO << "try to evaluate inst: " << inst->print();
    lattice val;
    if (inst->is_phi()) {
        LOG_ERROR << "Phi should be evaluated in other function!";
        exit(-1);
    }
    if (inst->is_add()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() +
                         std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    if (inst->is_sub()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() -
                         std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    // 针对乘法，因为乘法可以通过一个操作数完整确定值，因此需要重写结合规则
    // TODO 加入右侧操作数是否为 0 的判断，防止因为源码的除0导致 RE
    if (inst->is_mul()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        // top meet bottom = top
        if ((value[lhs].state == 1 and value[rhs].state == -1) or (value[lhs].state == -1 and value[rhs].state == 1))
            lat = 1;
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() *
                         std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];

        // 特殊情况，一个值就可以确定操作数值
        if (value[lhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
        if (value[rhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
    }
    // 类似乘法，由于 0/任何数都是 0，因此做了特殊处理
    if (inst->is_div()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        // top meet bottom = top
        if ((value[lhs].state == 1 and value[rhs].state == -1) or (value[lhs].state == -1 and value[rhs].state == 1))
            lat = 1;
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() /
                         std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];

        // 特殊情况，一个值就可以确定操作数值
        if (value[lhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
        if (value[rhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value() == 0) {
            LOG_ERROR << "Division by zero found!";
            val = {0, ConstantInt::get(0, m_), false};
        }
    }
    if (inst->is_srem()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        // top meet bottom = top
        if ((value[lhs].state == 1 and value[rhs].state == -1) or (value[lhs].state == -1 and value[rhs].state == 1))
            lat = 1;
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() %
                         std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];

        // 特殊情况，一个值就可以确定操作数值
        if (value[lhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
        if (value[rhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value() == 0) {
            LOG_ERROR << "Module by zero found!";
            val = {0, ConstantInt::get(0, m_), false};
        }
    }
    if (inst->is_shl()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        // top meet bottom = top
        if ((value[lhs].state == 1 and value[rhs].state == -1) or (value[lhs].state == -1 and value[rhs].state == 1))
            lat = 1;
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = (std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value()
                          << std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value());
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];

        // 特殊情况，一个值就可以确定操作数值
        if (value[lhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
        if (value[rhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value() >= 32) {
            LOG_ERROR << "SHL more than 31 bit found!";
            val = {0, ConstantInt::get(0, m_), false};
        }
    }
    // 算数右移不做特殊处理
    if (inst->is_ashr()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = (std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() >>
                          std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value());
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    // TODO 与运算不做处理
    if (inst->is_and_()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = (std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() &
                          std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value());
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    // 逻辑右移处理 0 和 >>32 的特殊情况
    if (inst->is_lshr()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        // top meet bottom = top
        if ((value[lhs].state == 1 and value[rhs].state == -1) or (value[lhs].state == -1 and value[rhs].state == 1))
            lat = 1;
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            int result = static_cast<int>(static_cast<unsigned>(
                std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() >>
                static_cast<unsigned>(std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value())));
            val = {0, ConstantInt::get(result, m_), false};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];

        // 特殊情况，一个值就可以确定操作数值
        if (value[lhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value() == 0)
            val = {0, ConstantInt::get(0, m_), false};
        if (value[rhs].state == 0 and std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value() >= 32) {
            LOG_ERROR << "LSHR more than 31 bit found!";
            val = {0, ConstantInt::get(0, m_), false};
        }
    }
    if (inst->is_fadd()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            float result = std::dynamic_pointer_cast<ConstantFP>(value[lhs].val)->get_value() +
                           std::dynamic_pointer_cast<ConstantFP>(value[rhs].val)->get_value();
            val = {0, ConstantFP::get(result, m_), true};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    if (inst->is_fsub()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            float result = std::dynamic_pointer_cast<ConstantFP>(value[lhs].val)->get_value() -
                           std::dynamic_pointer_cast<ConstantFP>(value[rhs].val)->get_value();
            val = {0, ConstantFP::get(result, m_), true};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    // 浮点数乘法因为有精度误差和 NAN 之类的特殊情况，暂时不对 0 进行特殊处理
    if (inst->is_fmul()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            float result = std::dynamic_pointer_cast<ConstantFP>(value[lhs].val)->get_value() *
                           std::dynamic_pointer_cast<ConstantFP>(value[rhs].val)->get_value();
            val = {0, ConstantFP::get(result, m_), true};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    if (inst->is_fdiv()) {
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            float result = std::dynamic_pointer_cast<ConstantFP>(value[lhs].val)->get_value() /
                           std::dynamic_pointer_cast<ConstantFP>(value[rhs].val)->get_value();
            val = {0, ConstantFP::get(result, m_), true};
        } else if (value[lhs].state == 0)
            val = value[lhs];
        else
            val = value[rhs];
    }
    if (inst->is_fp2si()) {
        auto op = inst->get_operand(0)->shared_from_this();
        if (value[op].state != 0)
            val = value[op];
        else {
            int result = static_cast<int>(std::dynamic_pointer_cast<ConstantFP>(value[op].val)->get_value());
            val = {0, ConstantInt::get(result, m_), false};
        }
    }
    if (inst->is_si2fp()) {
        auto op = inst->get_operand(0)->shared_from_this();
        if (value[op].state != 0)
            val = value[op];
        else {
            float result = static_cast<float>(std::dynamic_pointer_cast<ConstantInt>(value[op].val)->get_value());
            val = {0, ConstantFP::get(result, m_), true};
        }
    }
    if (inst->is_cmp()) {
        auto cmp_inst = std::dynamic_pointer_cast<CmpInst>(inst);
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            bool result = 0;
            int lval = std::dynamic_pointer_cast<ConstantInt>(value[lhs].val)->get_value();
            int rval = std::dynamic_pointer_cast<ConstantInt>(value[rhs].val)->get_value();
            switch (cmp_inst->get_cmp_op()) {
                case CmpOp::EQ:
                    result = (lval == rval);
                    break;
                case CmpOp::NE:
                    result = (lval != rval);
                    break;
                case CmpOp::GT:
                    result = (lval > rval);
                    break;
                case CmpOp::GE:
                    result = (lval >= rval);
                    break;
                case CmpOp::LT:
                    result = (lval < rval);
                    break;
                case CmpOp::LE:
                    result = (lval <= rval);
                    break;
                default:
                    LOG_ERROR << "strange cmp op";
                    exit(-1);
            }
            val = {0, ConstantInt::get(result, m_), false};
        } else
            val = {1, nullptr, false};  // 判断指令的话，如果操作数不确定赋予常数格值
    }
    if (inst->is_fcmp()) {
        auto cmp_inst = std::dynamic_pointer_cast<CmpInst>(inst);
        auto lhs = inst->get_operand(0)->shared_from_this();
        auto rhs = inst->get_operand(1)->shared_from_this();
        auto lat = std::min(value[lhs].state, value[rhs].state);
        if (lat != 0)
            val = {lat, nullptr};
        else if (value[lhs].state == 0 and value[rhs].state == 0) {
            bool result = 0;
            float lval = std::dynamic_pointer_cast<ConstantFP>(value[lhs].val)->get_value();
            float rval = std::dynamic_pointer_cast<ConstantFP>(value[rhs].val)->get_value();
            switch (cmp_inst->get_cmp_op()) {
                case CmpOp::EQ:
                    result = (lval == rval);
                    break;
                case CmpOp::NE:
                    result = (lval != rval);
                    break;
                case CmpOp::GT:
                    result = (lval > rval);
                    break;
                case CmpOp::GE:
                    result = (lval >= rval);
                    break;
                case CmpOp::LT:
                    result = (lval < rval);
                    break;
                case CmpOp::LE:
                    result = (lval <= rval);
                    break;
                default:
                    LOG_ERROR << "strange cmp op";
                    exit(-1);
            }
            val = {0, ConstantInt::get(result, m_), false};
        } else
            val = {1, nullptr, false};
    }

    LOG_INFO << "evaluate result: " << inst->print() << " , lattice is: " << val.print();

    return val;
}

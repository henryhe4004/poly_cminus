#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Module.h"
#include "Type.h"
#include "User.h"
#include "Value.h"

std::string print_as_op(Value *v, bool print_ty);
std::string print_cmp_type(CmpOp op);
std::string print_fcmp_type(CmpOp op);

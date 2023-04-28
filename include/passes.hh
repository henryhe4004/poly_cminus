#pragma once
#include "AdHocOptimization.hh"
#include "AlgebraicSimplification.hh"
#include "Branch.hh"
#include "DeadCode.hh"
#include "DeadGlobalElim.hh"
#include "DivisionInvariant.hh"
#include "FuncInfo.hh"
#include "FuncInline.hh"
#include "GepElim.hh"
// #include "GlobalCommonExpression.hh"
#include "GlobalVarLocal.hh"
#include "IVReduction.hh"
#include "InstructionSimplify.hh"
#include "LocalCommonExpression.hh"
#include "LoopInfo.hh"
#include "LoopInvMotion.hh"
#include "LoopMerge.hh"
#include "LoopSearch.hh"
#include "LoopUnrolling.hh"
#include "TransformSelect.hh"
#include "LowerIR.hh"
#include "Mem2Reg.hh"
#include "MulWeaken.hh"
#include "Pass.hh"
#include "Peephole.hh"
#include "RemoveUselessBr.hh"
#include "SCCP.hh"
#include "SimplifyCFG.hh"
#include "TailRecursionElim.hh"
#include "UselessOperationEli.hh"
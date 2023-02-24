#pragma once

#include "logging.hpp"

#include <cstdlib>

enum EXITCODE {
    ABNORMAL_ERROR = 2,
    MANIPULATE_INST_WITHOUT_PARENT,
    ERROR_IN_IR_CHECK,
    ERROR_IN_GENERATING_IR,
    ERROR_IN_ALGE_SIMP,
    ERROR_IN_PURE_FUNCTION_ANALYSIS,
    ERROR_IN_SCCP,
    ERROR_IN_CONST_FOLD,
    ERROR_IN_SIMP_CFG,
    UTILS_CALCULATE_TYPE_ERROR,
    ERROR_IN_GLOBAL_VAR_LOCAL_GLO_USER_ERROR,
    ERROR_IN_DCE,
    ERROR_IN_LCE,
    CODEGEN_ERROR,
    AA_TYPE_ERROR,
    POLY_CODEGEN_UNHANDLED_INST
};

inline void exit_if(bool cond, EXITCODE ret) {
    if (cond)
        exit(ret);
}

inline void exit_if(bool cond, EXITCODE ret, std::string msg) {
    if (cond) {
        LOG_ERROR << msg;
        exit(ret);
    }
}

inline void exit_ifnot(bool cond, EXITCODE ret) {
    if (not cond)
        exit(ret);
}

inline void exit_ifnot(bool cond, EXITCODE ret, std::string msg) {
    if (not cond) {
        LOG_ERROR << msg;
        exit(ret);
    }
}

inline void exit(EXITCODE ret, std::string msg) { exit_if(true, ret, msg); }

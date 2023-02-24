#ifndef PASS_HH
#define PASS_HH

#include "Module.h"
#include "errorcode.hh"
#include "logging.hpp"
#include "utils.hh"

#include <chrono>
#include <vector>

using std::shared_ptr;
using std::vector;

class Pass {
  public:
    Pass(Module *m) : m_(m) {}
    virtual void run() = 0;
    virtual ~Pass() = default;

  protected:
    Module *m_;
};

class IRCheck : public Pass {
  public:
    IRCheck(Module *m) : Pass(m) {}
    ~IRCheck(){};
    void run() override;

  private:
    void check_parent();
    void check_phi_position();
    void check_ret_br_position();
    void check_terminate();
    void check_pred_succ();
    void check_entry();
    void check_use_list();
    void check_operand_exit();
    void check_pred_succ_br();
    void check_correct_phi();
    void check_ssa();
};

class PassManager {
  private:
    Module *m_;
    IRCheck ir_check;
    bool debug_ir;

  public:
    vector<std::pair<std::shared_ptr<Pass>, bool>> pass_list;
    PassManager(Module *m) : m_(m), ir_check(m) {}

    template <typename T>
    void add_pass(bool print_IR = false) {
        pass_list.push_back({std::make_shared<T>(m_), print_IR});
    }

    void set_debug() { debug_ir = true; }

    void run() {
        for (auto [pass, print_IR] : pass_list) {
            auto &ref = *pass;
            // typeid 的执行不应该导致副作用，但是因为他的参数有解引用的操作，而这可能带来副作用，因此产生了警告
            auto pass_name = std::string(typeid(ref).name());
            LOG_DEBUG << "Running " << pass_name;
            std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            if (print_IR or debug_ir)
                LOG_DEBUG << "Before pass: " << pass_name << "\n" << m_->print(true);
            pass->run();
            std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
            std::chrono::duration<double> time_used =
                std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
            LOG_INFO << "Pass run time: " << pass_name << " uses " << time_used.count() << " seconds";
            if (print_IR or debug_ir)
                LOG_DEBUG << "After pass: " << pass_name << "\n" << m_->print(true);

// #ifdef LOCAL_TEST
//             try {
//                 LOG_DEBUG << "running IRCheck after pass " << pass_name;
//                 if (config.ir_check)
//                     ir_check.run();
//             } catch (...) {
//                 LOG_ERROR << "IRCheck ERROR after pass " << pass_name;
//                 LOG_DEBUG << "ERROR IR: \n" << m_->print(true);
//                 exit(ERROR_IN_IR_CHECK);
//             }
// #endif
        }
    }
};
#endif

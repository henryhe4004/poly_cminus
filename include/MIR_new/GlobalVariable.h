//
// Created by cqy on 2020/6/29.
//

#ifndef SYSYC_GLOBALVARIABLE_H
#define SYSYC_GLOBALVARIABLE_H

#include "Constant.h"
#include "Module.h"
#include "User.h"

class GlobalVariable : public User {
  private:
    bool is_const_;
    std::shared_ptr<Constant> init_val_;
    GlobalVariable(std::string name, Module *m, Type *ty, bool is_const, std::shared_ptr<Constant> init = nullptr);

  public:
    static std::shared_ptr<GlobalVariable> create(std::string name,
                                                  Module *m,
                                                  Type *ty,
                                                  bool is_const,
                                                  std::shared_ptr<Constant> init);

    std::shared_ptr<Constant> get_init() { return init_val_; }
    bool is_const() { return is_const_; }
    std::string print();
};
#endif  // SYSYC_GLOBALVARIABLE_H

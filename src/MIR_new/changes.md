1. delete BaseBlock (base class for BasicBlock) and HighBlock (including IfBlock, WhileBlock)
2. 增加 [CRTP](https://en.cppreference.com/w/cpp/language/crtp) (or should I say an interface) [BaseInst](../../include/MIR_new/Instruction.h)，用于批量添加工厂模式。
3. `Type`由`Module`独占所有权，`User`中的操作数类型为

```cc
struct operand_t {
    std::variant<std::shared_ptr<Value>, std::weak_ptr<Value>> operand;
};
```
shared_ptr 指向常量，变量，指令等等操作数；weak_ptr 指向BasicBlock, Function等等可能会导致循环引用的类型

4. TODO: change `print()`
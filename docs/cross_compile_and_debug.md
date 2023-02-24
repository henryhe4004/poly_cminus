# 交叉编译与调试
## prereq
需要安装
```
sudo apt install gcc-arm-linux-gnueabihf qemu-user
```
开启编译选项`-S`时（最好同时开启`-mem2reg`和`-gep-elim`），会生成三个文件，一个.ll，一个.s，和一个arm的二进制。ll文件是在寄存器分配之前print的，.s文件由后端生成，二进制则由`arm-linux-gnueabihf-gcc`交叉编译得到。

在x86下运行arm二进制需要qemu模拟器
```
qemu-arm a.out
```
## 一些命令
（不能用test.s, test.sy, test.ll等等名字是因为`test`是目录...）
1. 从.s编译到二进制
```
arm-linux-gnueabihf-gcc main.s lib/sylib.c -o main -static
qemu-arm main
```
2. 从.ll编译到arm汇编
```
clang -S --target=armv7-linux-gnueabihf main.ll -no-integrated-as
```
3. 从.ll编译到二进制
```
clang --target=armv7-linux-gnueabihf main.ll lib/sylib.c -fuse-ld=lld --sysroot=/usr/arm-linux-gnueabihf -static -o main
qemu-arm main
```
或者，从.ll到.s再到二进制
```
clang -S --target=armv7-linux-gnueabihf main.ll -no-integrated-as
arm-linux-gnueabihf-gcc main.s lib/sylib.c -o main -static
qemu-arm main
```
以及clang的参考输出
```
clang main.c lib/sylib.c -Ilib -fsanitize=undefined -fsanitize=address
./a.out
```
## debug
因为gep指令消除在x86上使用的是ptrtoint ... to i64，有些优化没有处理64位宽的数（开启-S之后ptrtoint是转换到i32，所以不会有这个问题），需要绕一下从arm平台判断优化正确性

如果.ll编译得到的结果没错，那么这是后端的错误
### 减小测试用例
使用creduce
### debug arm program
（如果确定是后端的错，直接跟我说就行了，不需要自己调试）

在x86上debug需要安装gdb-multiarch，为了使用vscode方便进行调试，设置launch.json
```json
{
    "name": "GDB debug - custom",
    "type": "cppdbg",
    "request": "launch",
    "program": "/home/dczhang/compiler/main",
    "args": [],
    "stopAtEntry": true,
    "cwd": "${workspaceFolder}",
    "environment": [],
    "externalConsole": false,
    "MIMode": "gdb",
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        }
    ],
    "miDebuggerPath": "gdb-multiarch",
    "miDebuggerServerAddress": "localhost:1234"
},
```
运行
```
qemu-arm -g 1234 main
```
然后按F5

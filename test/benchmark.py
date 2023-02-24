import argparse
import atexit
import os
import sqlite3
import subprocess
import sys
import timeit
import shutil
from fileinput import close
from typing import List
from unittest.result import failfast
import time

abs = os.path.dirname(os.path.abspath(__file__))
cc = os.path.join(abs, '../build/cc')
performance_test_dir = os.path.join(
    abs, "./compiler2022/公开样例与运行时库/performance")
functional_test_dir = os.path.join(
    abs, "./compiler2022/公开样例与运行时库/functional")
hidden_test_dir = os.path.join(
    abs, "./compiler2022/公开样例与运行时库/hidden_functional")
test_result = os.path.join(abs, './test_result')
benchmarkout = os.path.join(abs, './benchmark.out')
opt_args = os.path.join(abs, './opt-args')
total_start = None
# 测试时禁用日志
test_env = os.environ.copy()
test_env['LOGV'] = "5"
# print(abs, cc)
total_failed_count = 0


class Outputer:
    def __init__(self, console=False, filename="test_result") -> None:
        self.console = console
        self.fd = open(filename, "a")

    def write(self, msg):
        if self.console:
            print(msg, end="")
            sys.stdout.flush()
        self.fd.write(msg)
        self.fd.flush()

    def __del__(self) -> None:
        self.fd.close()


table_def = '''
create table TestResults(
    TestName text not null,
    TestTime int not null,
    TimeCost float,
    Result int not null,        /*0:success,1:tle,2:mismatch,3:dns,-1:other*/
    Options text,
    CommitID text,
    BackEnd int,
    Branch text,
    constraint pk_TestResult primary key(TestName,TestTime)
);
'''
"""
表定义：
    TestName text not null: 样例名称，不能为空
    TestTime int not null: 测试时间戳，不能为空
    TimeCost float: 样例运行耗时
    Result int not null: 运行结果，0:success,1:tle,2:mismatch,3:dns,-1:other，不能为空
    Options text: cc的命令行选项
    CommitID text: 当次的commit id
    BackEnd int: 是否使用自己写的后端
    Branch text: 分支名称
    主键为样例名称和时间戳
"""


class ResultDB:
    def __init__(self, path: str):
        try:
            self.db: sqlite3.Connection = sqlite3.connect(path)
        except Exception as e:
            print(e)
        self.cursor: sqlite3.Cursor = self.db.cursor()
        try:
            self.cursor.execute(table_def)
        except sqlite3.OperationalError as e:
            # print(e)
            pass
        atexit.register(self.db.commit)
        # atexit.register(self.db.close)

    def insert(self, *, name: str, test_time: int, result: int, cost: float = 1000.0, opts: List[str] = [], com_id: str = '', back_end: int = 0, branch: str = '') -> bool:
        '''
        必须以关键字参数形式传参
        '''
        try:
            self.cursor.execute("insert into TestResults (TestName,TestTime,Result,TimeCost,Options,CommitID,BackEnd,Branch) values(?,?,?,?,?,?,?,?)",
                                (name, test_time, result, cost, " ".join(opts), com_id, back_end, branch))
        except:
            return False
        self.db.commit()
        return True

    def commit(self) -> None:
        self.db.commit()

    # def close(self):
    #     self.db.close()
    #     atexit.unregister(self.commit)


def eval(opts, console=False, test_dirs=[functional_test_dir],
         output_file_name="test_result",
         time_cost=False,
         dbpath="benchmark_results.db", use_clang=False, use_qemu=False):
    test_time = int(time.time())
    database = ResultDB(dbpath)
    failed_count = 0
    compile_tle = 0
    exec_tle = 0
    output_file = Outputer(console, output_file_name)
    total_time = 0
    succ_count = 0
    single_begin = timeit.default_timer()
    for test_dir in test_dirs:
        output_file.write('============')
        output_file.write(test_dir)
        output_file.write('==========\n')
        func_test = os.path.join(abs, test_dir)
        tests = os.listdir(func_test)
        tests.sort()
        sy = filter(lambda s: s.endswith('.sy'), tests)
        sy = list(sy)
        test_count = len(sy)

        for count, f in enumerate(sy):
            # 设定总运行时间上限为 1 小时，上调单个样例的时限
            total_now = timeit.default_timer()
            if total_now-total_start > 30*60 or total_now-single_begin > 30*60:
                output_file.write(
                    f"[{count+1}/{test_count}] "+f + ': skipped due to exceeded total time limit\n')
                continue
            output_file.write(f"[{count+1}/{test_count}] "+f + ': ')
            filepath = os.path.join(func_test, f)
            outpath = os.path.join(func_test, f[:-3] + '.out')
            # print(outpath)

            try:
                if not use_clang:
                    compile_res = subprocess.run(
                        [cc, filepath, '-o', 'a.out'] + opts, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300, env=test_env)
                else:
                    cfilepath = filepath.replace(".sy", ".c")
                    shutil.move(filepath, cfilepath)
                    compile_res = subprocess.run(["clang", cfilepath, os.path.join(abs, "../build/lib/libsylib.a"), os.path.join(
                        abs, "../build/lib/libclanghelperlib.a"), "-static", "-o", "a.out", "-Ofast"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300)
                    shutil.move(cfilepath, filepath)
            except subprocess.TimeoutExpired as _:
                output_file.write('compile timeout\n')
                failed_count += 1
                compile_tle += 1
                if test_dir == performance_test_dir:
                    database.insert(name=f, test_time=test_time,
                                    result=3, opts=opts)
                continue
            except Exception:
                output_file.write("compile failed with an unexcept error\n")
                failed_count += 1
                if test_dir == performance_test_dir:
                    database.insert(name=f, test_time=test_time,
                                    result=3, opts=opts)
                continue
            if compile_res.returncode != 0:
                output_file.write(
                    f'build/cc returns {compile_res.returncode} instead of 0\n')
                failed_count += 1
                if test_dir == performance_test_dir:
                    database.insert(name=f, test_time=test_time,
                                    result=3, opts=opts)
                continue
            try:
                inpath = os.path.join(func_test, f[:-3] + '.in')
                input_option = None
                if os.path.exists(inpath):
                    with open(inpath, 'rb') as fin:
                        input_option = fin.read()
                start = timeit.default_timer()
                if not use_qemu:
                    exe_res = subprocess.run(
                        ['./a.out'], input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300)
                else:
                    exe_res = subprocess.run(
                        ['qemu-arm', 'a.out'], input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300)
                end = timeit.default_timer()
            except subprocess.TimeoutExpired:
                output_file.write("executable time limit exceeded\n")
                failed_count += 1
                exec_tle += 1
                if test_dir == performance_test_dir:
                    database.insert(name=f, test_time=test_time,
                                    result=1, opts=opts)
                continue
            except Exception as _:
                output_file.write('executable runtime error\n')
                failed_count += 1
                if test_dir == performance_test_dir:
                    database.insert(name=f, test_time=test_time,
                                    result=-1, opts=opts)
                continue
            with open(outpath, 'r') as o:
                ref = o.read().replace(' ', '').replace('\n', '')
                try:
                    actual = exe_res.stdout.decode(
                    'utf-8').replace(' ', '').replace('\n', '').replace('\r', '') + str(exe_res.returncode)
                except UnicodeDecodeError:
                    output_file.write('executable output illegal characters\n')
                    failed_count+=1
                    if test_dir == performance_test_dir:
                        database.insert(name=f, test_time=test_time,
                                        result=-1, opts=opts)
                    continue
                if ref == actual or use_clang:
                    if time_cost:
                        output_file.write(f'pass, costs {end-start:.2f}s\n')
                    else:
                        output_file.write("pass\n")
                    succ_count += 1
                    total_time += end-start
                    if test_dir == performance_test_dir:
                        database.insert(name=f, test_time=test_time,
                                        result=0, cost=end - start, opts=opts)
                else:
                    output_file.write(
                        'output is different from standard answer, this may be caused by wrong return code\n')   # 因为退出码也会作为输出的一部分，因此输出和答案不同可能是程序崩溃造成的
                    if test_dir == performance_test_dir:
                        database.insert(name=f, test_time=test_time,
                                        result=2, opts=opts)
                    failed_count += 1

    output_file.write(f"{failed_count} tests failed\n")
    output_file.write(
        f"optimiztion is {opts}\ntotal time is {total_time}s\navg time is {total_time/succ_count if succ_count>0 else 0}s\n{succ_count} tests finishes in time limit\n")
    output_file.write(
        f"{compile_tle} files compiled tle\n{exec_tle} exectuables tle\n")
    global total_failed_count
    total_failed_count += failed_count


if __name__ == '__main__':
    total_start = timeit.default_timer()
    parser = argparse.ArgumentParser(description="functional test")
    parser.add_argument("-mem2reg", "-m", action="append_const",
                        const="-mem2reg", help="open mem2reg pass", dest="opts")
    parser.add_argument("--console", action="store_true",
                        help="specify whether to output the result to console")
    parser.add_argument("--time", action="store_true",
                        help="calculate run time for each file")
    parser.add_argument("--func", action="store_true",
                        help="test on functional test cases")
    parser.add_argument("--dbpath",
                        help="path of database")
    parser.add_argument("--custom", nargs="*",
                        help="relative path of custom test cases")
    parser.add_argument("--clang", action="store_true",
                        help="estimate runtime when compile with clang")
    parser.add_argument("--arm", action="store_true",
                        help="use qemu-arm to run program")
    parser.add_argument("--hidden", action="store_true",
                        help="enable hidden functional test")
    args = parser.parse_args()

    opts = args.opts if args.opts is not None else []
    test_dir = []
    if args.func:
        test_dir.append(functional_test_dir)
    if args.time:
        test_dir.append(performance_test_dir)
    if args.hidden:
        test_dir.append(hidden_test_dir)
    if args.custom and len(args.custom) > 0:
        test_dir += args.custom
    if os.path.exists(benchmarkout):
        os.remove(benchmarkout)
    with open(opt_args, "r") as f:
        for cmd in f:
            add_opt = cmd.strip().split()
            eval(opts+add_opt, args.console, test_dir,
                 benchmarkout, True, dbpath="benchmark_results.db" if args.dbpath is None else args.dbpath, use_clang=args.clang, use_qemu=args.arm)
    exit(total_failed_count)

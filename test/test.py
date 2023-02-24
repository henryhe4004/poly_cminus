import os
import subprocess
import sys
import argparse
import platform
from concurrent.futures import ProcessPoolExecutor, wait as wait_futures, Future
from typing import List, Dict, Tuple, Iterable

abs = os.path.dirname(os.path.abspath(__file__))
cc = os.path.join(abs, '../build/cc')
opt_args = os.path.join(abs, './opt-args')
functional_test_dir = os.path.join(
    abs, "./compiler2022/公开样例与运行时库/functional")
h_functional_test_dir = os.path.join(
    abs, "./compiler2022/公开样例与运行时库/hidden_functional")


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


class Eval:
    def __init__(self, max_workers: int):
        self.executor = ProcessPoolExecutor(max_workers=max_workers)
        self.function_tests_abs = [functional_test_dir, h_functional_test_dir]
        self.result: Dict[str, Dict[str, Future]] = {}
        self.futures: List[Future] = []

    def eval(self, opts: List[str], console: bool = False):
        result_s: Dict[str, List[str]] = {}
        for func_test in self.function_tests_abs:
            self.result[func_test] = {}
            result_s[func_test] = []
            func_test_path = os.path.join(abs, func_test)
            tests = os.listdir(func_test_path)
            sy = filter(lambda s: s.endswith('.sy'), tests)
            # sy = ['00_main.sy']
            for f in sy:
                fu = self.executor.submit(self._eval, f, func_test_path, opts)
                self.result[func_test][f] = fu
                self.futures.append(fu)
        wait_futures(self.futures)
        self.failed_count = 0
        for year, funcs in self.result.items():
            def _count_and_cat(item: Tuple[str, Future]):
                if item[1].result() != "pass\n":
                    self.failed_count += 1
                return f"{item[0]}: {item[1].result()}"
            result_s[year] = list(map(_count_and_cat, funcs.items()))
            result_s[year].sort()
        output_file = Outputer(console, "test_result")
        for func_test in self.function_tests_abs:
            output_file.write('============')
            output_file.write(func_test)
            output_file.write('==========\n')
            for res in result_s[func_test]:
                output_file.write(res)
        if self.failed_count > 0:
            print(f'============failed_count: {self.failed_count}')
        exit(self.failed_count)

    @staticmethod
    def _eval(f: str, func_test: str, opts: List[str]):
        ft = f[:-3]
        filepath = os.path.join(func_test, f)
        outpath = filepath[:-3] + '.out'
        exepath = os.path.join(func_test, ft)
        try:
            compile_res = subprocess.run(
                [cc, filepath] + opts + ['-o', exepath], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=100, env=dict(os.environ, LOGV='4'))
        except subprocess.TimeoutExpired:
            return "compile time out\n"
        except Exception as _:
            return 'compile failed due to unknown reason\n'
        if compile_res.returncode != 0:
            return f'build/cc returns {compile_res.returncode} instead of 0\n'
        try:
            inpath = os.path.join(filepath[:-3] + '.in')
            input_option = None
            if os.path.exists(inpath):
                with open(inpath, 'rb') as fin:
                    input_option = fin.read()
            if '-S' in opts and 'arm' not in platform.machine():
                exe_command = ['qemu-arm-static', exepath]
            else:
                exe_command = [exepath]
            exe_res = subprocess.run(
                exe_command, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
            os.remove(exepath)
            with open(outpath, 'r') as o:
                ref = o.read().replace(' ', '').replace('\n', '')
                actual = exe_res.stdout.decode(
                    'utf-8').replace(' ', '').replace('\n', '').replace('\r', '') + str(exe_res.returncode)
                if ref == actual:
                    return 'pass\n'
                else:
                    return f'output and returncode mismatch, result is {actual} but should be {ref}\n'
        except subprocess.TimeoutExpired:
            return 'executable time out\n'
        except Exception as _:
            return 'executable runtime error\n'


if __name__ == '__main__':
    if os.path.exists("test_result"):
        os.remove("test_result")

    parser = argparse.ArgumentParser(description="fanctional test")
    parser.add_argument("-mem2reg", "-m", action="append_const",
                        const="-mem2reg", help="open mem2reg pass", dest="opts", default=[])
    parser.add_argument("--console", action="store_true",
                        help="specify whether to output the result to console")
    parser.add_argument("-j", type=int, help="multi-process limit", default=1)
    args = parser.parse_args()
    # opts = args.opts if args.opts is not None else []
    with open(opt_args, "r") as f:
        for cmd in f:
            add_opt = cmd.strip().split()
            e = Eval(args.j)
            print('args:', cmd)
            e.eval(args.opts+add_opt, args.console)

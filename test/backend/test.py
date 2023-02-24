import os
import subprocess
import re
import sys


# find all files in directories and subdirectories, use os.walk
def findAllFiles(path):
    result = []
    for root, dirs, files in os.walk(path):
        for file in files:
            result.append(os.path.join(root, file))
    return result


allFiles = findAllFiles('.')

allFiles.sort()

compareWithGiven = False

if len(sys.argv) > 1 and sys.argv[1] == '-c':
    compareWithGiven = True

complierFilePath = '../../build/cc'
if compareWithGiven:
    libFilePath = '../../lib/sylib.c'
    libHeaderPath = '../../lib/sylib.h'
else:
    libFilePath = '../../lib/sylib_debug.c'
    libHeaderPath = '../../lib/sylib_debug.h'

for file in allFiles:
    if file.endswith('.sy'):
        runFileName = file.replace('.sy', '.o')
        sFileName = file.replace('.sy', '.s')
        txtFileName = file.replace('.sy', '.in')
        ansFileName = file.replace('.sy', '.out')
        cFileName = file.replace('.sy', '.c')

        os.system(f'rm {sFileName} > /dev/null 2>&1')
        os.system(f'rm {runFileName} > /dev/null 2>&1')

        compileCommand = f'{complierFilePath} {file} -o {runFileName} -S -mem2reg -gep-elim'

        # os.system(f'{compileCommand} > /dev/null 2>&1')
        # os.system(f'arm-linux-gnueabihf-gcc -static {sFileName} {libFilePath} -o {runFileName}')

        print(f'Compiling {file}')

        p1 = subprocess.run(compileCommand, shell=True, capture_output=True, timeout=30)
        if p1.returncode != 0:
            sys.stderr.write(f'Compiling {file} failed\n')
            sys.stderr.write(p1.stdout.decode('utf-8'))
            continue

        p2 = subprocess.run(f'arm-linux-gnueabihf-gcc -static {sFileName} {libFilePath} -o {runFileName}',
                            shell=True,
                            capture_output=True,
                            timeout=10)
        if p2.returncode != 0:
            sys.stderr.write(f'Linking {file} failed\n')
            sys.stderr.write(p2.stdout.decode('utf-8'))
            continue

        # txtFileName as input if exist
        if os.path.exists(txtFileName):
            inputFile = open(txtFileName, 'r')
        else:
            inputFile = None
        command = f'qemu-arm {runFileName}'
        try:
            process1 = subprocess.run(command, shell=True, stdin=inputFile, capture_output=True, timeout=10)
        except:
            sys.stderr.write(f'Running {file} failed\n')
            continue
        output1 = process1.stdout.decode('utf-8')
        returnCode1 = process1.returncode

        ans1 = output1 + str(returnCode1)

        # os.system(f'rm {sFileName}')
        # os.system(f'rm {runFileName}')

        # if compareWithGiven:
        #     with open(ansFileName, 'r') as ansFile:
        #         ans2 = ansFile.read()
        # else:
        #     if os.path.exists(txtFileName):
        #         inputFile = open(txtFileName, 'r')
        #     else:
        #         inputFile = None

        #     os.system(f'cp {file} {cFileName}')
        #     compileCommand2 = f'arm-linux-gnueabihf-gcc -static {cFileName} {libFilePath} -o {runFileName} -include {libHeaderPath}'
        #     compileProcess = subprocess.run(compileCommand2, shell=True)
        #     if compileProcess.returncode != 0:
        #         print(f'{file} compile error')
        #         continue
        #     process2 = subprocess.run(f'qemu-arm {runFileName}', shell=True, stdin=inputFile, capture_output=True)
        #     output2 = process2.stdout.decode('utf-8')
        #     returnCode2 = process2.returncode

        #     ans2 = output2 + str(returnCode2)
        #     os.system(f'rm {runFileName}')

        # compare ans1 and ans2, ignore ' ' '\n' '\t'
        # ans1 = re.sub(r'\s', '', ans1)
        # ans2 = re.sub(r'\s', '', ans2)
        # if ans1 == ans2:
        #     print(f'{file} pass')
        # else:
        #     print(f'{file} fail')
        #     exit(1)

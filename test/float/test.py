import os
import subprocess
import sys

from sympy import N

libFilePath = '../../lib/sylib_debug.c'
libHeaderPath = '../../lib/sylib_debug.h'
complierFilePath = '../../build/cc'

allFiles = os.listdir('.')

allFiles.sort()

for file in allFiles:
    if file.endswith('.c'):
        outFileName = file.replace('.c', '.out')
        sFileName = file.replace('.c', '.s')
        txtFileName = file.replace('.c', '.in')

        os.system(f'{complierFilePath} {file} -o {outFileName} -S -mem2reg -gep-elim > /dev/null 2>&1')
        os.system(f'arm-linux-gnueabihf-gcc -static {sFileName} {libFilePath} -o {outFileName}')

        # txtFileName as input if exist
        if os.path.exists(txtFileName):
            inputFile = open(txtFileName, 'r')
        else:
            inputFile = None
        process1 = subprocess.run(f'qemu-arm {outFileName}', shell=True, stdin=inputFile, capture_output=True)
        output1 = process1.stdout.decode('utf-8')
        returnCode1 = process1.returncode

        if os.path.exists(txtFileName):
            inputFile = open(txtFileName, 'r')
        else:
            inputFile = None

        os.system(f'rm {outFileName}')

        os.system(f'arm-linux-gnueabihf-gcc -static {file} {libFilePath} -o {outFileName} -include {libHeaderPath}')
        process2 = subprocess.run(f'qemu-arm {outFileName}', shell=True, stdin=inputFile, capture_output=True)
        output2 = process2.stdout.decode('utf-8')
        returnCode2 = process2.returncode

        os.system(f'rm {outFileName}')

        if returnCode1 == returnCode2 and output1 == output2:
            print(f'{file} is correct')
        else:
            print(f'{file} is wrong')
            sys.stderr.write(f'{output1}{returnCode1}\n\n')
            sys.stderr.write(f'{output2}{returnCode2}\n')

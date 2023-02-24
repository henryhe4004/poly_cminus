import subprocess
import os

allFailed = []


def printRedBold(text):
    print(f'\033[1;31m{text}\033[0m')


def testDir(directory):
    mainFile = '../../build/cc'
    allTestFile = os.listdir(directory)
    allTestFile.sort()
    for testFile in allTestFile:
        if testFile.endswith('.sy'):
            testFilePath = os.path.join(directory, testFile)
            command = f'{mainFile} {testFilePath}'
            # subprocess, get output and error, and return code
            process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            # print(process.stdout.decode('utf-8'))
            # print(process.stderr.decode('utf-8'))
            if process.returncode != 0 or process.stderr != b'':
                printRedBold(f'{testFilePath} failed')
                os.system(f'cp {testFilePath} error.c')
                allFailed.append(f'{testFilePath}')
                # exit(1)
            else:
                print(f'{testFilePath}')


def main():
    testDir('../function_test2020')
    testDir('../function_test2021')
    # testDir('../performance_test2021-private')
    # testDir('../performance_test2021-public')

    printRedBold(allFailed)


main()

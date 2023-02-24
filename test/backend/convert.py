import os
import re


# find all files in directories and subdirectories, use os.walk
def findAllFiles(path):
    result = []
    for root, dirs, files in os.walk(path):
        for file in files:
            result.append(os.path.join(root, file))
    return result


allFiles = findAllFiles('.')

allFiles.sort()

for file in allFiles:
    if file.endswith('.sy'):
        with open(file, 'r') as f:
            content = f.read()

        # content = content.replace('int', 'float')
        # content = content.replace('float main', 'int main')

        allIndex = re.findall(r'\[(.*?)\]', content)
        for index in allIndex:
            content = content.replace(f'float {index}', f'int {index}')

        with open(file, 'w') as f:
            f.write(content)

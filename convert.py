import os
import re


# list all files under subdirectories
def list_all_files(directory):
    allFiles = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            allFiles.append(os.path.join(root, file))
    return allFiles


allFiles = list_all_files('.')

index = 1

for file in allFiles:
    # replace 'exit(150), LOG(ERROR)' to 'exit({index}), exit(151), LOG(ERROR)'
    # index += 1
    if file.endswith('.cc'):
        try:
            with open(file, 'r') as f:
                content = f.read()

            # if 'exit(152), LOG(ERROR)' in content:
            #     allParts = content.split('exit(150), exit(154), LOG(ERROR)')
            #     newContent = allParts[0]
            #     for i in range(1, len(allParts)):
            #         newContent += f'exit({index}), exit(154), LOG(ERROR)'
            #         newContent += allParts[i]
            #         index += 1

            #     with open(file, 'w') as f:
            #         f.write(newContent)

            if ' assert(' in content:
                content = re.sub(r' assert\(([\s\S]+?)\);', r'if (!(\1)) exit(-);', content)
                allParts = content.split('exit(-);')
                newContent = allParts[0]
                for i in range(1, len(allParts)):
                    newContent += f'exit({index});'
                    newContent += allParts[i]
                    index += 1

                with open(file, 'w') as f:
                    f.write(newContent)
        except Exception as e:
            print(file)

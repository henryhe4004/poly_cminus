import re

with open('parser_raw.y', 'r') as f:
    lines = f.readlines()

for i in range(len(lines)):
    matched = re.match(r'([A-Za-z_]+) +: (.*?);', lines[i])
    if matched:
        name = matched.group(1)
        expressions = matched.group(2).split(' | ')
        if name != 'CompUnit':
            for j in range(len(expressions)):
                num = len(re.findall('\w+', expressions[j]))
                dollars = ', '.join([f'${i}' for i in range(1, num + 1)])
                # expressions[j] += ' { $$ = new_node("CompUnit", {$1, $2}); }'
                expressions[j] += f' {{ $$ = new_node("{name}", {{{dollars}}}); }}'

            joinExpression = ' | '.join(expressions)
            lines[i] = f'{name} : {joinExpression};\n'
outContent = ''.join(lines)
outFile = open('parser.y', 'w')
outFile.write(outContent)

flex --header-file=lexer.hh -o lexer.cc lexer.l

python3 generateParser.py
bison -d -o parser.cc parser.y
rm parser.y

mv lexer.hh ../../include/parser/
mv parser.hh ../../include/parser/

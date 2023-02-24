#include "SyntaxTree.hh"
#include "lexer.hh"
#include "logging.hpp"
#include "parser.hh"

#include <fstream>
#include <iostream>

std::string preprocess(const std::string &f) {
    // 处理 starttime 和 stoptime 宏定义的问题
    std::string ret{};
    ret.reserve(f.size());
    int now = 0, linecount = 1;
    while (now < f.size()) {
        auto pos = std::min(f.find("starttime()", now), f.find("stoptime()", now));
        if (pos == std::string::npos) {
            ret += f.substr(now);
            break;
        }
        for (int i = now; i < pos; ++i)
            linecount += f[i] == '\n';
        ret += f.substr(now, pos - now);
        now = pos;
        if (f.substr(now, 10) == "stoptime()") {
            ret += std::string{"_sysy_stoptime("} + std::to_string(linecount) + std::string{")"};
            now += 10;
        } else {
            ret += std::string{"_sysy_starttime("} + std::to_string(linecount) + std::string{")"};
            now += 11;
        }
    }

    // LOG_INFO << f << "\n------------------\n" << ret;

    return ret;
}

void parse_file(string input_file_path) {
    const char *input_file_path_cstr = input_file_path.c_str();
    if (input_file_path != "") {
        auto input_file = fopen(input_file_path_cstr, "r");
        if (!input_file) {
            cout << "Error: Cannot open file " << input_file_path_cstr << "\n";
            exit(110);
        }
        std::ifstream t(input_file_path);
        std::string buffer;

        t.seekg(0, std::ios::end);
        buffer.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        buffer.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

        // fseek(input_file, 0L, SEEK_END);
        // auto input_size = ftell(input_file);
        // rewind(input_file);

        // char *buffer = new char[input_size + 10];
        // fread(buffer, 1, input_size + 5, input_file);
        // FILE *buffer_file = fmemopen(buffer, strlen(buffer), "r");
        // LOG_DEBUG << buffer;
        auto preprocessed = preprocess(buffer);
        // LOG_DEBUG << preprocessed;

        yy_scan_string(preprocessed.c_str());
        // yyrestart(buffer_file);
    }
    yyparse();
}

SyntaxTreeNode *new_node(string name, vector<SyntaxTreeNode *> children = {}) {
    // cout << "New: " << name << "\n";
    auto node = new SyntaxTreeNode(name, children, nullptr);
    for (auto child : children)
        child->parent = node;
    return node;
}

void delete_all_children_node(SyntaxTreeNode *root) {
    vector<SyntaxTreeNode *> all_node;
    all_node.push_back(root);
    while (all_node.size() > 0) {
        auto node = all_node.back();
        all_node.pop_back();
        for (auto child : node->children)
            all_node.push_back(child);
        delete node;
    }
}

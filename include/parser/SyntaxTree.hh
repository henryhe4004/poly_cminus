#ifndef SYNTAX_TREE_HH
#define SYNTAX_TREE_HH

#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::string;
using std::vector;

class SyntaxTreeNode {
  public:
    string name;
    SyntaxTreeNode *parent;
    vector<SyntaxTreeNode *> children;

    SyntaxTreeNode(string name, vector<SyntaxTreeNode *> children, SyntaxTreeNode *parent) {
        this->name = name;
        this->children = children;
        this->parent = parent;
    }

    void print(int layer = 0) {
        cout << string(4 * layer, ' ') << name << "\n";
        for (auto child : children)
            child->print(layer + 1);
    }
};

class SyntaxTree {
  public:
    SyntaxTreeNode *root;

    void print() { root->print(); }
};

void parse_file(string input_file_path);

SyntaxTreeNode *new_node(string name, vector<SyntaxTreeNode *> children);

void delete_all_children_node(SyntaxTreeNode *root);

#endif

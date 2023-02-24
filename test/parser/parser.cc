#include "driver.hh"

#include <iostream>

int main(int argc, char *argv[]) {
    int res = 0;
    driver drv;
    if (argc <= 1) {
        std::cerr << "Usage: " << argv[0] << " [-p] [-s] <filename>" << std::endl;
        return -1;
    }
    for (int i = 1; i < argc; ++i)
        if (argv[i] == std::string("-p"))
            drv.trace_parsing = true;
        else if (argv[i] == std::string("-s"))
            drv.trace_scanning = true;
        else if (!drv.parse(argv[i]))
            std::cout << drv.root << '\n';
        else
            res = 1;
    return res;
}

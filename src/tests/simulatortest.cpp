#include "Simulator.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

static std::string lowered(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " program.limo\n"; return 1; }
    std::ifstream file(argv[1]);
    if (!file) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }
    std::stringstream text; text << file.rdbuf();

    limo::Simulator sim;
    try {
        sim.load(limo::assemble(text.str()));
    } catch (const limo::AsmError& e) {
        std::cerr << "Assembly error: " << e.what() << "\n";
        return 1;
    }
    std::cout << "LIMO simulator ready. Type 'help'.\n";

    std::string line;
    while (std::cout << "limo> " && std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd; in >> cmd; cmd = lowered(cmd);
        if (cmd == "mohato" || cmd == "step") {
            long n = 1; in >> n;
            for (long i = 0; i < n && sim.step(std::cout); ++i) {}
        } else if (cmd == "mathisa" || cmd == "run") { sim.run(std::cout);
        } else if (cmd == "qalong" || cmd == "reset") { sim.reset(std::cout);
        } else if (cmd == "regs") { sim.printRegisters(std::cout);
        } else if (cmd == "mem")  { sim.printMemory(std::cout);
        } else if (cmd == "list") { sim.printListing(std::cout);
        } else if (cmd == "help") {
            std::cout << "  mohato|step [n]  advance n instructions (default 1)\n"
                         "  mathisa|run      run until the program ends\n"
                         "  qalong|reset     PC=0, registers=0, data memory cleared, program reloaded\n"
                         "  regs  mem  list  help  quit\n";
        } else if (cmd == "quit" || cmd == "exit") { break;
        } else if (!cmd.empty()) { std::cout << "Unknown command. Type 'help'.\n"; }
    }
    return 0;
}

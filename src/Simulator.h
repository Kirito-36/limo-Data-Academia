#pragma once

#include "Assembler.h"
#include "CPU.h"
#include <iosfwd>

namespace limo {

class Simulator {
public:
    void load(const AssembledProgram& program);    
    bool step(std::ostream& out);                   
    std::uint64_t run(std::ostream& out,           
                      std::uint64_t maxSteps = 10000000);
    void reset(std::ostream& out);                  

    bool finished() const;
    void printRegisters(std::ostream& out) const;
    void printMemory(std::ostream& out) const;
    void printListing(std::ostream& out) const;
    std::uint64_t executed() const { return executed_; }

private:
    bool execute(std::ostream& out, bool verbose);
    void installProgram();

    CPU cpu_;
    AssembledProgram program_;
    std::uint64_t executed_ = 0;
    bool faulted_ = false;
};

}  

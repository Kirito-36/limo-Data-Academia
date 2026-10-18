#pragma once

#include "cpp.h"
#include "Memory.h"
#include "RegisterFile.h"

namespace limo {

class CPU {
public:
    explicit CPU(std::uint32_t entryPc = 0u);

    void reset();
    void loadProgram(const std::vector<std::uint8_t>& programBytes, std::uint32_t startAddress = 0u);
    void writeProgramWord(std::uint32_t address, std::uint32_t word);

    RegisterFile& registers();
    Memory& memory();
    std::uint32_t pc() const;
    bool step();

private:
    RegisterFile registers_;
    Memory memory_;
    std::uint32_t pc_ = 0u;
};

}  // namespace limo

#pragma once

#include "cpp.h"
#include "Memory.h"
#include "PipelineRegisters.h"
#include "RegisterFile.h"

namespace limo {

// Five-stage LIMO pipeline: IF, ID, EX, MEM, WB.
//
// tick() runs one clock cycle. Stages are evaluated back to front (WB, MEM,
// EX, ID, IF), each reading the *current* pipeline registers and producing the
// *next* ones; all four registers are latched together at the end of the tick.
// WB runs before ID, so a value written back this cycle is already visible to
// the register read in ID (write in the first half, read in the second).
//
// Branches resolve in EX. A taken branch redirects the PC to
// (branch address + imm_B) and turns the two younger instructions (in ID and
// being fetched) into bubbles.
//
// Not included yet (hazard unit): forwarding and load-use stalls. Dependent
// instructions need NOPs between them until that exists.
class Pipeline {
public:
    explicit Pipeline(std::uint32_t entryPc = 0u);

    void reset();
    void loadProgram(const std::vector<std::uint8_t>& programBytes, std::uint32_t startAddress = 0u);
    void writeProgramWord(std::uint32_t address, std::uint32_t word);

    // Fetch stops (inserts bubbles) once pc >= endAddress. loadProgram sets
    // this to the end of the loaded program.
    void setProgramEnd(std::uint32_t endAddress);

    RegisterFile& registers();
    Memory& memory();
    std::uint32_t pc() const;

    void tick();
    // Ticks until fetch has reached the program end and the pipeline is empty,
    // or maxCycles is hit. Returns the number of cycles run.
    std::uint64_t run(std::uint64_t maxCycles = 1000000u);
    bool empty() const;

    const IfId& ifId() const;
    const IdEx& idEx() const;
    const ExMem& exMem() const;
    const MemWb& memWb() const;

    std::uint64_t cycles() const;
    std::uint64_t retired() const;       // instructions that reached WB
    std::uint64_t illegalCount() const;  // words decoded as NOP (doc section 1.4)

private:
    void clearPipeline();

    void stageWriteBack(const MemWb& in);
    MemWb stageMemory(const ExMem& in);
    ExMem stageExecute(const IdEx& in, bool& redirect, std::uint32_t& target) const;
    IdEx stageDecode(const IfId& in);
    IfId stageFetch();

    RegisterFile registers_;
    Memory memory_;
    std::uint32_t pc_ = 0u;
    std::uint32_t programEnd_ = 0xFFFFFFFFu;

    IfId ifId_;
    IdEx idEx_;
    ExMem exMem_;
    MemWb memWb_;

    std::uint64_t cycles_ = 0u;
    std::uint64_t retired_ = 0u;
    std::uint64_t illegal_ = 0u;
};

}  // namespace limo

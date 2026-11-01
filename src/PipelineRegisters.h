#pragma once

#include "cpp.h"
#include "ControlUnit.h"

namespace limo {

// A register with valid == false is a bubble: every control signal is 0, so it
// changes no architectural state as it moves down the pipeline.

struct IfId {
    bool valid = false;
    std::uint32_t pc = 0u;
    std::uint32_t instr = 0u;
};

struct IdEx {
    bool valid = false;
    std::uint32_t pc = 0u;
    std::uint32_t instr = 0u;
    Op op = Op::NOP;
    std::uint8_t rs1 = 0u, rs2 = 0u, rd = 0u;  // kept for the hazard unit
    std::uint32_t rs1Val = 0u, rs2Val = 0u;
    std::int32_t imm = 0;
    ExControl ex;
    MemControl mem;
    WbControl wb;
};

struct ExMem {
    bool valid = false;
    std::uint32_t pc = 0u;
    std::uint32_t instr = 0u;
    Op op = Op::NOP;
    std::uint32_t aluResult = 0u;  // ALU result / effective address
    std::uint32_t storeData = 0u;  // rs2 value, used by beha
    std::uint8_t rd = 0u;
    MemControl mem;
    WbControl wb;
};

struct MemWb {
    bool valid = false;
    std::uint32_t pc = 0u;
    std::uint32_t instr = 0u;
    Op op = Op::NOP;
    std::uint32_t aluResult = 0u;
    std::uint32_t memData = 0u;
    std::uint8_t rd = 0u;
    WbControl wb;
};

}  // namespace limo

#pragma once

#include "cpp.h"

namespace limo {

// The ten LIMO instructions (M1 ISA, docs/CONTROL_UNIT_AND_ALU_LIMO.md section 1.3).
// NOP is what everything else decodes to (section 1.4).
enum class Op : std::uint8_t {
    NOP,
    KOPANYA,   // add
    TLOSA,     // sub
    KAPA,      // or
    LE,        // and
    EKETSA,    // addi
    SUTHISA,   // slli
    BALA,      // load word
    BEHA,      // store word
    LEKANYA,   // branch if equal
    PHAPANYO   // branch if not equal
};

// ALU_Control = {funct7[5]_eff, funct3} (doc section 3.1).
namespace alu_ctl {
constexpr std::uint8_t ADD = 0x0u;  // 0000
constexpr std::uint8_t SLL = 0x1u;  // 0001
constexpr std::uint8_t OR  = 0x6u;  // 0110
constexpr std::uint8_t AND = 0x7u;  // 0111
constexpr std::uint8_t SUB = 0x8u;  // 1000
}  // namespace alu_ctl

// Control signals grouped by the stage that consumes them (doc section 2.1).
// ID/EX carries all three groups, EX/MEM carries mem + wb, MEM/WB carries wb.
struct ExControl {
    bool aluSrc = false;           // 1: operand B is the immediate
    bool branch = false;
    bool branchOnNotZero = false;  // funct3[0]: phapanyo (1) vs lekanya (0)
    std::uint8_t aluOp = 0;        // 00 load/store, 01 branch, 10 R, 11 I-arith
    std::uint8_t aluControl = 0;   // from aluControl(), see alu_ctl
};

struct MemControl {
    bool memRead = false;
    bool memWrite = false;
};

struct WbControl {
    bool regWrite = false;
    bool memToReg = false;
};

// Result of the ID-stage decode: register numbers (4-bit), immediate and controls.
struct DecodedControl {
    Op op = Op::NOP;
    bool legal = false;  // false -> decoded as NOP (debug "illegal" output)
    std::uint8_t rd = 0;
    std::uint8_t rs1 = 0;
    std::uint8_t rs2 = 0;
    std::int32_t imm = 0;
    ExControl ex;
    MemControl mem;
    WbControl wb;
};

class ControlUnit {
public:
    // Decodes a raw instruction word exactly as the M1 control-unit document
    // describes: 4-bit register fields, reserved bits must be 0, anything that
    // is not one of the ten patterns is a NOP with every control signal at 0.
    static DecodedControl decode(std::uint32_t word);

    // ALU control unit (doc section 4).
    static std::uint8_t aluControl(std::uint8_t aluOp, std::uint8_t funct3, bool funct7Bit5);

    static const char* opName(Op op);
};

}  // namespace limo

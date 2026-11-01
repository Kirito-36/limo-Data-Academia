#include "ControlUnit.h"

namespace limo {

namespace {

inline std::int32_t signExtend(std::uint32_t value, unsigned bits) {
    const unsigned shift = 32u - bits;
    return static_cast<std::int32_t>(value << shift) >> shift;
}

inline std::int32_t immI(std::uint32_t w) {
    return signExtend((w >> 20) & 0xFFFu, 12);
}

inline std::int32_t immS(std::uint32_t w) {
    return signExtend(((w >> 25) & 0x7Fu) << 5 | ((w >> 7) & 0x1Fu), 12);
}

inline std::int32_t immB(std::uint32_t w) {
    const std::uint32_t v = ((w >> 31) & 0x1u) << 12 |
                            ((w >> 7) & 0x1u) << 11 |
                            ((w >> 25) & 0x3Fu) << 5 |
                            ((w >> 8) & 0xFu) << 1;
    return signExtend(v, 13);
}

}  // namespace

std::uint8_t ControlUnit::aluControl(std::uint8_t aluOp, std::uint8_t funct3, bool funct7Bit5) {
    switch (aluOp) {
    case 0x0u:
        return alu_ctl::ADD;  // bala, beha: address = base + offset
    case 0x1u:
        return alu_ctl::SUB;  // lekanya, phapanyo: compare by subtracting
    default: {
        // 10 = R-type, 11 = I-type. funct7[5] only matters for R-type add/sub;
        // for eketsa it is just bit 10 of the immediate and must be ignored.
        const bool f7Eff = (aluOp == 0x2u && funct3 == 0x0u) ? funct7Bit5 : false;
        return static_cast<std::uint8_t>((f7Eff ? 0x8u : 0x0u) | (funct3 & 0x7u));
    }
    }
}

DecodedControl ControlUnit::decode(std::uint32_t w) {
    const std::uint8_t opcode = static_cast<std::uint8_t>(w & 0x7Fu);
    const std::uint8_t funct3 = static_cast<std::uint8_t>((w >> 12) & 0x7u);
    const std::uint8_t funct7 = static_cast<std::uint8_t>((w >> 25) & 0x7Fu);

    // Reserved top bits of the 5-bit-wide fields (doc section 1.2).
    const bool bit11 = ((w >> 11) & 1u) != 0u;  // rd[4]
    const bool bit19 = ((w >> 19) & 1u) != 0u;  // rs1[4]
    const bool bit24 = ((w >> 24) & 1u) != 0u;  // rs2[4]

    Op op = Op::NOP;
    switch (opcode) {
    case 0x33u:  // R-type
        if (!bit11 && !bit19 && !bit24) {
            if (funct3 == 0x0u && funct7 == 0x00u)      op = Op::KOPANYA;
            else if (funct3 == 0x0u && funct7 == 0x20u) op = Op::TLOSA;
            else if (funct3 == 0x6u && funct7 == 0x00u) op = Op::KAPA;
            else if (funct3 == 0x7u && funct7 == 0x00u) op = Op::LE;
        }
        break;
    case 0x13u:  // I-type arithmetic. inst[24] is imm[4]/shamt[4] here, so it is allowed.
        if (!bit11 && !bit19) {
            if (funct3 == 0x0u)                         op = Op::EKETSA;
            else if (funct3 == 0x1u && funct7 == 0x00u) op = Op::SUTHISA;
        }
        break;
    case 0x03u:  // load
        if (!bit11 && !bit19 && funct3 == 0x2u) op = Op::BALA;
        break;
    case 0x23u:  // store: inst[11] is imm[4]
        if (!bit19 && !bit24 && funct3 == 0x2u) op = Op::BEHA;
        break;
    case 0x63u:  // branch: inst[11] is imm[11]
        if (!bit19 && !bit24) {
            if (funct3 == 0x0u)      op = Op::LEKANYA;
            else if (funct3 == 0x1u) op = Op::PHAPANYO;
        }
        break;
    default:
        break;
    }

    DecodedControl d;
    d.op = op;
    d.legal = (op != Op::NOP);
    if (!d.legal) {
        return d;  // NOP: all fields and controls stay 0
    }

    const std::uint8_t rd  = static_cast<std::uint8_t>((w >> 7) & 0xFu);
    const std::uint8_t rs1 = static_cast<std::uint8_t>((w >> 15) & 0xFu);
    const std::uint8_t rs2 = static_cast<std::uint8_t>((w >> 20) & 0xFu);

    // Main control unit (doc section 2) plus the register fields each format really uses.
    switch (op) {
    case Op::KOPANYA:
    case Op::TLOSA:
    case Op::KAPA:
    case Op::LE:
        d.rd = rd; d.rs1 = rs1; d.rs2 = rs2;
        d.ex.aluSrc = false; d.ex.aluOp = 0x2u;
        d.wb.regWrite = true;
        break;
    case Op::EKETSA:
    case Op::SUTHISA:
        d.rd = rd; d.rs1 = rs1;
        d.imm = immI(w);
        d.ex.aluSrc = true; d.ex.aluOp = 0x3u;
        d.wb.regWrite = true;
        break;
    case Op::BALA:
        d.rd = rd; d.rs1 = rs1;
        d.imm = immI(w);
        d.ex.aluSrc = true; d.ex.aluOp = 0x0u;
        d.mem.memRead = true;
        d.wb.regWrite = true; d.wb.memToReg = true;
        break;
    case Op::BEHA:
        d.rs1 = rs1; d.rs2 = rs2;
        d.imm = immS(w);
        d.ex.aluSrc = true; d.ex.aluOp = 0x0u;
        d.mem.memWrite = true;
        break;
    case Op::LEKANYA:
    case Op::PHAPANYO:
        d.rs1 = rs1; d.rs2 = rs2;
        d.imm = immB(w);
        d.ex.aluSrc = false; d.ex.aluOp = 0x1u;
        d.ex.branch = true;
        d.ex.branchOnNotZero = (funct3 & 0x1u) != 0u;
        break;
    case Op::NOP:
        break;
    }

    d.ex.aluControl = aluControl(d.ex.aluOp, funct3, (funct7 & 0x20u) != 0u);
    return d;
}

const char* ControlUnit::opName(Op op) {
    switch (op) {
    case Op::NOP:      return "NOP";
    case Op::KOPANYA:  return "KOPANYA";
    case Op::TLOSA:    return "TLOSA";
    case Op::KAPA:     return "KAPA";
    case Op::LE:       return "LE";
    case Op::EKETSA:   return "EKETSA";
    case Op::SUTHISA:  return "SUTHISA";
    case Op::BALA:     return "BALA";
    case Op::BEHA:     return "BEHA";
    case Op::LEKANYA:  return "LEKANYA";
    case Op::PHAPANYO: return "PHAPANYO";
    }
    return "UNKNOWN";
}

}  // namespace limo

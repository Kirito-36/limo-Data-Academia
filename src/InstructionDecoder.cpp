#include "InstructionDecoder.h"
#include <cstdint>

namespace limo {

namespace {
    // sign-extend helper for n-bit value in lower bits of value
    inline std::int32_t signExtend(uint32_t value, unsigned bits) {
        const unsigned shift = 32 - bits;
        return static_cast<std::int32_t>(value << shift) >> shift;
    }
}

DecodedInstruction InstructionDecoder::decode(std::uint32_t word) {
    DecodedInstruction di;
    di.raw = word;

    di.opcode = static_cast<std::uint8_t>(word & 0x7F);
    di.rd     = static_cast<std::uint8_t>((word >> 7) & 0x1F);
    di.funct3 = static_cast<std::uint8_t>((word >> 12) & 0x07);
    di.rs1    = static_cast<std::uint8_t>((word >> 15) & 0x1F);
    di.rs2    = static_cast<std::uint8_t>((word >> 20) & 0x1F);
    di.funct7 = static_cast<std::uint8_t>((word >> 25) & 0x7F);

    // immediates
    di.immI = signExtend((word >> 20) & 0xFFFu, 12);
    // S: imm[11:5] = bits[31:25], imm[4:0] = bits[11:7]
    {
        uint32_t immS = ((word >> 25) & 0x7Fu) << 5;
        immS |= ((word >> 7) & 0x1Fu);
        di.immS = signExtend(immS, 12);
    }
    // B: imm[12] = bit31, imm[11]=bit7, imm[10:5]=bits30:25, imm[4:1]=bits11:8, imm[0]=0
    {
        uint32_t immB = ((word >> 31) & 0x1u) << 12;
        immB |= ((word >> 7) & 0x1u) << 11;
        immB |= ((word >> 25) & 0x3Fu) << 5;
        immB |= ((word >> 8) & 0xFu) << 1;
        di.immB = signExtend(immB, 13);
    }

    // Determine instruction format from opcode (RV32I-style)
    switch (di.opcode) {
    // R-type opcodes (OP)
    case 0x33:
        di.format = InstructionFormat::R;
        di.valid = true;
        break;
    // I-type opcodes (loads, immediate ALU, JALR)
    case 0x03: // LOAD
    case 0x13: // OP-IMM
    case 0x67: // JALR (if present)
        di.format = InstructionFormat::I;
        di.valid = true;
        break;
    // S-type opcodes (STORE)
    case 0x23:
        di.format = InstructionFormat::S;
        di.valid = true;
        break;
    // B-type opcodes (BRANCH)
    case 0x63:
        di.format = InstructionFormat::B;
        di.valid = true;
        break;
    default:
        di.format = InstructionFormat::INVALID;
        di.valid = false;
        break;
    }

    // Map concrete opcode/funct3/funct7 values to M1 mnemonics.
    // Mapping below uses common RV32I encodings. Replace if M1.pdf specifies different encodings.
    if (di.valid) {
        if (di.format == InstructionFormat::R) {
            // OP opcode 0x33
            if (di.funct3 == 0x0 && di.funct7 == 0x00) {
                // ADD
                di.mnemonic = M1Mnemonic::KOPANYA; // ADD -> KOPANYA
            } else if (di.funct3 == 0x0 && di.funct7 == 0x20) {
                // SUB
                di.mnemonic = M1Mnemonic::TLOSA; // SUB -> TLOSA
            } else if (di.funct3 == 0x7 && di.funct7 == 0x00) {
                // AND
                di.mnemonic = M1Mnemonic::KAPA; // AND -> KAPA
            } else if (di.funct3 == 0x2 && di.funct7 == 0x00) {
                // SLT
                di.mnemonic = M1Mnemonic::LEKANYA; // SLT -> LEKANYA
            } else {
                di.mnemonic = M1Mnemonic::INVALID;
            }
        } else if (di.format == InstructionFormat::I) {
            if (di.opcode == 0x13) {
                // OP-IMM
                if (di.funct3 == 0x0) {
                    // ADDI
                    di.mnemonic = M1Mnemonic::EKETSA; // ADDI -> EKETSA
                } else if (di.funct3 == 0x1) {
                    // SLLI (I-shift)
                    di.mnemonic = M1Mnemonic::SUTHISA; // SLLI -> SUTHISA
                } else {
                    di.mnemonic = M1Mnemonic::INVALID;
                }
            } else if (di.opcode == 0x03) {
                // LOADs
                if (di.funct3 == 0x2) {
                    // LW
                    di.mnemonic = M1Mnemonic::LE; // LW -> LE
                } else {
                    di.mnemonic = M1Mnemonic::INVALID;
                }
            } else {
                di.mnemonic = M1Mnemonic::INVALID;
            }
        } else if (di.format == InstructionFormat::S) {
            // STORE
            if (di.funct3 == 0x2) {
                // SW
                di.mnemonic = M1Mnemonic::PHAPANYO; // SW -> PHAPANYO
            } else {
                di.mnemonic = M1Mnemonic::INVALID;
            }
        } else if (di.format == InstructionFormat::B) {
            // Branches
            if (di.funct3 == 0x0) {
                // BEQ
                di.mnemonic = M1Mnemonic::BALA; // BEQ -> BALA
            } else if (di.funct3 == 0x1) {
                // BNE
                di.mnemonic = M1Mnemonic::BEHA; // BNE -> BEHA
            } else {
                di.mnemonic = M1Mnemonic::INVALID;
            }
        }
    }

    return di;
}

const char* InstructionDecoder::mnemonicName(M1Mnemonic mnemonic) {
    switch (mnemonic) {
    case M1Mnemonic::INVALID:   return "INVALID";
    case M1Mnemonic::KOPANYA:   return "KOPANYA";
    case M1Mnemonic::TLOSA:     return "TLOSA";
    case M1Mnemonic::EKETSA:    return "EKETSA";
    case M1Mnemonic::KAPA:      return "KAPA";
    case M1Mnemonic::LE:        return "LE";
    case M1Mnemonic::SUTHISA:   return "SUTHISA";
    case M1Mnemonic::BALA:      return "BALA";
    case M1Mnemonic::BEHA:      return "BEHA";
    case M1Mnemonic::LEKANYA:   return "LEKANYA";
    case M1Mnemonic::PHAPANYO:  return "PHAPANYO";
    default:                    return "UNKNOWN";
    }
}

} // namespace limo

#include "InstructionDecoder.h"

#include "ControlUnit.h"

#include <cstdint>

namespace limo {

namespace {
    // sign-extend helper for n-bit value in lower bits of value
    inline std::int32_t signExtend(std::uint32_t value, unsigned bits) {
        const unsigned shift = 32 - bits;
        return static_cast<std::int32_t>(value << shift) >> shift;
    }

    // ControlUnit::decode is the single place that knows which words are legal
    // LIMO instructions (docs/CONTROL_UNIT_AND_ALU_LIMO.md sections 1.2-1.4).
    // This function only translates its answer into the decoder's own types.
    M1Mnemonic toMnemonic(Op op) {
        switch (op) {
        case Op::KOPANYA:  return M1Mnemonic::KOPANYA;
        case Op::TLOSA:    return M1Mnemonic::TLOSA;
        case Op::KAPA:     return M1Mnemonic::KAPA;
        case Op::LE:       return M1Mnemonic::LE;
        case Op::EKETSA:   return M1Mnemonic::EKETSA;
        case Op::SUTHISA:  return M1Mnemonic::SUTHISA;
        case Op::BALA:     return M1Mnemonic::BALA;
        case Op::BEHA:     return M1Mnemonic::BEHA;
        case Op::LEKANYA:  return M1Mnemonic::LEKANYA;
        case Op::PHAPANYO: return M1Mnemonic::PHAPANYO;
        case Op::NOP:      break;
        }
        return M1Mnemonic::INVALID;
    }

    InstructionFormat toFormat(Op op) {
        switch (op) {
        case Op::KOPANYA:
        case Op::TLOSA:
        case Op::KAPA:
        case Op::LE:       return InstructionFormat::R;
        case Op::EKETSA:
        case Op::SUTHISA:
        case Op::BALA:     return InstructionFormat::I;
        case Op::BEHA:     return InstructionFormat::S;
        case Op::LEKANYA:
        case Op::PHAPANYO: return InstructionFormat::B;
        case Op::NOP:      break;
        }
        return InstructionFormat::INVALID;
    }

    Type toType(InstructionFormat format) {
        switch (format) {
        case InstructionFormat::R: return Type::R;
        case InstructionFormat::I: return Type::I;
        case InstructionFormat::S: return Type::S;
        case InstructionFormat::B: return Type::B;
        default:                   return Type::INVALID;
        }
    }
}

DecodedInstruction InstructionDecoder::decode(std::uint32_t word) {
    DecodedInstruction di;
    di.raw = word;

    di.opcode = static_cast<std::uint8_t>(word & 0x7F);
    di.funct3 = static_cast<std::uint8_t>((word >> 12) & 0x07);
    di.funct7 = static_cast<std::uint8_t>((word >> 25) & 0x7F);

    // LIMO registers are 4 bits wide; the 5th bit of each field is reserved.
    di.rd  = static_cast<std::uint8_t>((word >> 7) & 0x0F);
    di.rs1 = static_cast<std::uint8_t>((word >> 15) & 0x0F);
    di.rs2 = static_cast<std::uint8_t>((word >> 20) & 0x0F);

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

    // Anything that is not one of the ten LIMO instructions, or that has a
    // reserved bit set, comes back as INVALID (the pipeline executes it as a NOP).
    const DecodedControl ctl = ControlUnit::decode(word);
    if (ctl.legal) {
        di.mnemonic = toMnemonic(ctl.op);
        di.format = toFormat(ctl.op);
        di.type = toType(di.format);
        di.valid = true;
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

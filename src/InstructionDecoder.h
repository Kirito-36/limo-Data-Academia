#pragma once

#include <cstdint>

namespace limo {

enum class M1Mnemonic {
    INVALID,
    KOPANYA,
    TLOSA,
    EKETSA,
    KAPA,
    LE,
    SUTHISA,
    BALA,
    BEHA,
    LEKANYA,
    PHAPANYO
};

enum class InstructionFormat {
    R,
    I,
    S,
    B,
    INVALID
};

struct DecodedInstruction {
    M1Mnemonic mnemonic = M1Mnemonic::INVALID;
    InstructionFormat format = InstructionFormat::INVALID;

    std::uint32_t raw = 0;
    std::uint8_t opcode = 0;
    std::uint8_t funct3 = 0;
    std::uint8_t funct7 = 0;

    std::uint8_t rd = 0;
    std::uint8_t rs1 = 0;
    std::uint8_t rs2 = 0;

    std::int32_t immI = 0;
    std::int32_t immS = 0;
    std::int32_t immB = 0;

    bool valid = false;
};

class InstructionDecoder {
public:
    static DecodedInstruction decode(std::uint32_t word);
    static const char* mnemonicName(M1Mnemonic mnemonic);
};

}  // namespace limo

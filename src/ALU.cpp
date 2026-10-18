#include "ALU.h"

#include <cstdint>
#include <stdexcept>

namespace limo {

std::uint32_t ALU::executeR(std::uint8_t funct3, std::uint8_t funct7, std::uint32_t rs1, std::uint32_t rs2) {
    switch (funct3) {
        case 0x0u:
            return (funct7 == 0x20u) ? (rs1 - rs2) : (rs1 + rs2);
        case 0x1u:
            return rs1 << (rs2 & 0x1Fu);
        case 0x2u:
            return (static_cast<std::int32_t>(rs1) < static_cast<std::int32_t>(rs2)) ? 1u : 0u;
        case 0x3u:
            return (rs1 < rs2) ? 1u : 0u;
        case 0x4u:
            return rs1 ^ rs2;
        case 0x5u:
            return (funct7 == 0x20u) ? (static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1) >> (rs2 & 0x1Fu))) : (rs1 >> (rs2 & 0x1Fu));
        case 0x6u:
            return rs1 | rs2;
        case 0x7u:
            return rs1 & rs2;
        default:
            throw std::invalid_argument("Unsupported ALU R-type instruction");
    }
}

std::uint32_t ALU::executeI(std::uint8_t funct3, std::uint32_t rs1, std::int32_t imm) {
    switch (funct3) {
        case 0x0u:
            return static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1) + imm);
        case 0x1u:
            return rs1 << (imm & 0x1Fu);
        case 0x2u:
            return (static_cast<std::int32_t>(rs1) < imm) ? 1u : 0u;
        case 0x3u:
            return (static_cast<std::uint32_t>(rs1) < static_cast<std::uint32_t>(imm)) ? 1u : 0u;
        case 0x4u:
            return rs1 ^ static_cast<std::uint32_t>(imm);
        case 0x5u:
            return (imm < 0) ? (static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1) >> (static_cast<std::uint32_t>(imm) & 0x1Fu)))
                             : (rs1 >> (static_cast<std::uint32_t>(imm) & 0x1Fu));
        case 0x6u:
            return rs1 | static_cast<std::uint32_t>(imm);
        case 0x7u:
            return rs1 & static_cast<std::uint32_t>(imm);
        default:
            throw std::invalid_argument("Unsupported ALU I-type instruction");
    }
}

bool ALU::executeBranch(std::uint8_t funct3, std::uint32_t rs1, std::uint32_t rs2) {
    switch (funct3) {
        case 0x0u:
            return rs1 == rs2;
        case 0x1u:
            return rs1 != rs2;
        case 0x4u:
            return static_cast<std::int32_t>(rs1) < static_cast<std::int32_t>(rs2);
        case 0x5u:
            return static_cast<std::int32_t>(rs1) >= static_cast<std::int32_t>(rs2);
        case 0x6u:
            return rs1 < rs2;
        case 0x7u:
            return rs1 >= rs2;
        default:
            throw std::invalid_argument("Unsupported branch instruction");
    }
}

}  // namespace limo

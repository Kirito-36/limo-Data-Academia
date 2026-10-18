#pragma once

#include "cpp.h"

namespace limo {

class ALU {
public:
    static std::uint32_t executeR(std::uint8_t funct3, std::uint8_t funct7, std::uint32_t rs1, std::uint32_t rs2);
    static std::uint32_t executeI(std::uint8_t funct3, std::uint32_t rs1, std::int32_t imm);
    static bool executeBranch(std::uint8_t funct3, std::uint32_t rs1, std::uint32_t rs2);
};

}  // namespace limo

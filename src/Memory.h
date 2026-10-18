#pragma once

#include "cpp.h"

namespace limo {

class Memory {
public:
    Memory();

    void reset();

    void storeByte(std::uint32_t address, std::uint8_t value);
    void storeHalfWord(std::uint32_t address, std::uint16_t value);
    void storeWord(std::uint32_t address, std::uint32_t value);

    std::uint8_t loadByte(std::uint32_t address) const;
    std::uint16_t loadHalfWord(std::uint32_t address) const;
    std::uint32_t loadWord(std::uint32_t address) const;

    void writeRange(std::uint32_t startAddress, const std::vector<std::uint8_t>& bytes);

private:
    static constexpr std::size_t kMemorySize = 1u << 20;
    std::vector<std::uint8_t> storage_;
};

}  // namespace limo

#include "Memory.h"

#include <algorithm>
#include <stdexcept>
namespace limo {

Memory::Memory() : storage_(kMemorySize, 0u) {
}

void Memory::reset() {
    std::fill(storage_.begin(), storage_.end(), 0u);
}

void Memory::storeByte(std::uint32_t address, std::uint8_t value) {
    if (address >= storage_.size()) {
        throw std::out_of_range("Memory address out of range");
    }
    storage_[address] = value;
}

void Memory::storeHalfWord(std::uint32_t address, std::uint16_t value) {
    if ((address & 1u) != 0u) {
        throw std::invalid_argument("Half-word store requires 2-byte alignment");
    }
    storeByte(address, static_cast<std::uint8_t>(value & 0xFFu));
    storeByte(address + 1u, static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

void Memory::storeWord(std::uint32_t address, std::uint32_t value) {
    if ((address & 3u) != 0u) {
        throw std::invalid_argument("Word store requires 4-byte alignment");
    }
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        storeByte(address + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>((value >> (8u * i)) & 0xFFu));
    }
}

std::uint8_t Memory::loadByte(std::uint32_t address) const {
    if (address >= storage_.size()) {
        throw std::out_of_range("Memory address out of range");
    }
    return storage_[address];
}

std::uint16_t Memory::loadHalfWord(std::uint32_t address) const {
    if ((address & 1u) != 0u) {
        throw std::invalid_argument("Half-word load requires 2-byte alignment");
    }
    return static_cast<std::uint16_t>(loadByte(address)) |
           (static_cast<std::uint16_t>(loadByte(address + 1u)) << 8u);
}

std::uint32_t Memory::loadWord(std::uint32_t address) const {
    if ((address & 3u) != 0u) {
        throw std::invalid_argument("Word load requires 4-byte alignment");
    }
    std::uint32_t value = 0u;
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        value |= static_cast<std::uint32_t>(loadByte(address + static_cast<std::uint32_t>(i))) << (8u * i);
    }
    return value;
}

void Memory::writeRange(std::uint32_t startAddress, const std::vector<std::uint8_t>& bytes) {
    if (startAddress + static_cast<std::uint32_t>(bytes.size()) > storage_.size()) {
        throw std::out_of_range("Program range exceeds memory bounds");
    }
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        storage_[startAddress + static_cast<std::uint32_t>(i)] = bytes[i];
    }
}

}  // namespace limo

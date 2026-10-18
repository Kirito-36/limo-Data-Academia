#include "RegisterFile.h"

#include <algorithm>

namespace limo {

RegisterFile::RegisterFile(std::uint32_t initialPc) {
    reset();
    pc_ = initialPc;
}

void RegisterFile::reset() {
    registers_.fill(0u);
    pc_ = 0u;
}

std::uint32_t RegisterFile::read(std::uint8_t reg) const {
    if (reg == 0u) {
        return 0u;
    }
    if (reg >= registers_.size()) {
        throw std::out_of_range("Register index out of range");
    }
    return registers_[reg];
}

void RegisterFile::write(std::uint8_t reg, std::uint32_t value) {
    if (reg == 0u) {
        return;
    }
    if (reg >= registers_.size()) {
        throw std::out_of_range("Register index out of range");
    }
    registers_[reg] = value;
}

void RegisterFile::setPc(std::uint32_t pc) {
    pc_ = pc;
}

std::uint32_t RegisterFile::pc() const {
    return pc_;
}

}  // namespace limo

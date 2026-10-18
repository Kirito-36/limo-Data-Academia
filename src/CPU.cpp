#include "CPU.h"

#include "ALU.h"
#include "InstructionDecoder.h"

#include <stdexcept>

namespace limo {

CPU::CPU(std::uint32_t entryPc)
    : registers_(entryPc), pc_(entryPc) {
}

void CPU::reset() {
    registers_.reset();
    memory_.reset();
    pc_ = 0u;
}

void CPU::loadProgram(const std::vector<std::uint8_t>& programBytes, std::uint32_t startAddress) {
    memory_.writeRange(startAddress, programBytes);
    registers_.setPc(startAddress);
    pc_ = startAddress;
}

void CPU::writeProgramWord(std::uint32_t address, std::uint32_t word) {
    memory_.storeWord(address, word);
}

RegisterFile& CPU::registers() {
    return registers_;
}

Memory& CPU::memory() {
    return memory_;
}

std::uint32_t CPU::pc() const {
    return pc_;
}

bool CPU::step() {
    const std::uint32_t instructionWord = memory_.loadWord(pc_);
    const auto decoded = InstructionDecoder::decode(instructionWord);

    if (!decoded.valid) {
        throw std::runtime_error("Invalid instruction encountered in LIMO CPU");
    }

    switch (decoded.type) {
    case DecodedInstruction::Type::R: {
        const std::uint32_t rs1 = registers_.read(decoded.rs1);
        const std::uint32_t rs2 = registers_.read(decoded.rs2);
        const std::uint32_t result = ALU::executeR(decoded.funct3, decoded.funct7, rs1, rs2);
        if (decoded.rd != 0u) {
            registers_.write(decoded.rd, result);
        }
        pc_ += 4u;
        return true;
    }
    case DecodedInstruction::Type::I: {
        const std::uint32_t rs1 = registers_.read(decoded.rs1);
        const std::uint32_t result = ALU::executeI(decoded.funct3, rs1, decoded.immI);
        if (decoded.rd != 0u) {
            registers_.write(decoded.rd, result);
        }
        pc_ += 4u;
        return true;
    }
    case DecodedInstruction::Type::S: {
        const std::uint32_t rs1 = registers_.read(decoded.rs1);
        const std::uint32_t rs2 = registers_.read(decoded.rs2);
        const std::uint32_t address = rs1 + static_cast<std::uint32_t>(decoded.immS);
        memory_.storeWord(address, rs2);
        pc_ += 4u;
        return true;
    }
    case DecodedInstruction::Type::B: {
        const std::uint32_t rs1 = registers_.read(decoded.rs1);
        const std::uint32_t rs2 = registers_.read(decoded.rs2);
        const bool takeBranch = ALU::executeBranch(decoded.funct3, rs1, rs2);
        pc_ = takeBranch ? (pc_ + static_cast<std::uint32_t>(decoded.immB)) : (pc_ + 4u);
        return true;
    }
    default:
        throw std::runtime_error("Unsupported LIMO instruction type");
    }
}

}  // namespace limo

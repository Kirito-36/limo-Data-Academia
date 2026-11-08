#include "Simulator.h"

#include "InstructionDecoder.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace limo {
namespace {

const char* kNames[16] = {"noto", "lehae", "lesupa", "n1", "n2", "n3", "n4", "n5",
                          "n6", "k1", "k2", "k3", "k4", "p1", "p2", "p3"};
constexpr std::size_t kShownRegs = std::min<std::size_t>(16, detail::kRegisterCount);
constexpr std::uint32_t kDataBytes = 1u << 20;      // size of limo::Memory (kMemorySize)

struct Control { bool regWrite, memRead, memWrite, branch; bool aluImm; const char* alu; };

Control controlFor(M1Mnemonic m) {
    switch (m) {
    case M1Mnemonic::KOPANYA:  return {true,  false, false, false, false, "ADD"};
    case M1Mnemonic::TLOSA:    return {true,  false, false, false, false, "SUB"};
    case M1Mnemonic::KAPA:     return {true,  false, false, false, false, "OR"};
    case M1Mnemonic::LE:       return {true,  false, false, false, false, "AND"};
    case M1Mnemonic::EKETSA:   return {true,  false, false, false, true,  "ADD"};
    case M1Mnemonic::SUTHISA:  return {true,  false, false, false, true,  "SLL"};
    case M1Mnemonic::BALA:     return {true,  true,  false, false, true,  "ADD"};
    case M1Mnemonic::BEHA:     return {false, false, true,  false, true,  "ADD"};
    case M1Mnemonic::LEKANYA:  return {false, false, false, true,  false, "EQ"};
    case M1Mnemonic::PHAPANYO: return {false, false, false, true,  false, "NE"};
    default:                   return {false, false, false, false, false, "-"};
    }
}

std::string hex32(std::uint32_t v) {
    std::ostringstream o;
    o << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << v;
    return o.str();
}
std::int32_t s(std::uint32_t v) { return static_cast<std::int32_t>(v); }

} 

void Simulator::installProgram() {
    std::vector<std::uint8_t> bytes;
    for (std::uint32_t w : program_.words)
        for (int b = 0; b < 4; ++b) bytes.push_back(static_cast<std::uint8_t>((w >> (8 * b)) & 0xFFu));   
    cpu_.loadProgram(bytes, 0u);
}

void Simulator::load(const AssembledProgram& program) {
    program_ = program;
    cpu_.reset();
    installProgram();
    executed_ = 0;
    faulted_ = false;
}

void Simulator::reset(std::ostream& out) {
    cpu_.reset();            
    installProgram();       
    executed_ = 0;
    faulted_ = false;
    out << "Reset: PC = 0x00000000, all registers cleared to 0, data memory cleared, program reloaded.\n";
}

bool Simulator::finished() const {
    return cpu_.pc() / 4u >= program_.words.size();
}

bool Simulator::execute(std::ostream& out, bool verbose) {
    if (faulted_) { out << "The CPU stopped on an error. Use qalong (reset) to start again.\n"; return false; }
    if (finished()) { out << "Program finished (PC = " << hex32(cpu_.pc()) << "). Use qalong (reset) to run again.\n"; return false; }

    const std::uint32_t pc = cpu_.pc();
    const std::uint32_t word = cpu_.memory().loadWord(pc);
    const DecodedInstruction d = InstructionDecoder::decode(word);
    const Control c = controlFor(d.mnemonic);

    std::uint32_t before[16] = {0};
    for (std::size_t i = 0; i < kShownRegs; ++i) before[i] = cpu_.registers().read(static_cast<std::uint8_t>(i));

  
    std::uint32_t address = 0;
    if (d.mnemonic == M1Mnemonic::BALA) address = before[d.rs1 & 0xF] + static_cast<std::uint32_t>(d.immI);
    if (d.mnemonic == M1Mnemonic::BEHA) address = before[d.rs1 & 0xF] + static_cast<std::uint32_t>(d.immS);

    try {
        cpu_.step();
    } catch (const std::exception& e) {
        out << "Error at PC=" << pc << " (" << hex32(word) << "): " << e.what() << "\n";
        faulted_ = true;
        return false;
    }
    ++executed_;
    if (!verbose) return true;

    out << "PC=" << std::setw(3) << pc << "  " << hex32(word) << "  " << program_.source[pc / 4u] << "\n"
        << "        ctrl: RegWrite=" << c.regWrite << " MemRead=" << c.memRead << " MemWrite=" << c.memWrite
        << " Branch=" << c.branch << " ALUSrc=" << (c.aluImm ? "imm" : "reg") << " ALU=" << c.alu << "\n";

    if (c.regWrite && d.rd != 0) {
        const std::uint32_t after = cpu_.registers().read(d.rd);
        out << "        " << kNames[d.rd & 0xF] << ": " << s(before[d.rd & 0xF]) << " -> " << s(after);
        if (d.mnemonic == M1Mnemonic::BALA) out << "   (read Mem[" << address << "])";
        out << "\n";
    }
    if (c.memWrite)
        out << "        Mem[" << address << "] <- " << s(cpu_.dataMemory().loadWord(address)) << "\n";
    if (c.branch)
        out << "        branch " << (cpu_.pc() != pc + 4u ? "TAKEN" : "not taken") << ", next PC = " << cpu_.pc() << "\n";
    return true;
}

bool Simulator::step(std::ostream& out) { return execute(out, true); }

std::uint64_t Simulator::run(std::ostream& out, std::uint64_t maxSteps) {
    const std::uint64_t start = executed_;
    while (!finished() && !faulted_ && executed_ - start < maxSteps) {
        if (!execute(out, false)) break;
    }
    const std::uint64_t n = executed_ - start;
    if (faulted_)             out << "Run stopped on an error after " << n << " instructions.\n";
    else if (finished())      out << "Program finished after " << n << " instructions.\n";
    else                      out << "Stopped at the step limit (" << n << " instructions); the program is still running.\n";
    return n;
}

void Simulator::printRegisters(std::ostream& out) const {
    out << "PC = " << cpu_.pc() << "\n";
    for (std::size_t i = 0; i < kShownRegs; ++i) {
        out << "  r" << std::left << std::setw(2) << i << " " << std::setw(7) << kNames[i] << std::right
            << std::setw(11) << s(cpu_.registers().read(static_cast<std::uint8_t>(i)))
            << ((i % 2) ? "\n" : "   |");
    }
}

void Simulator::printMemory(std::ostream& out) const {
    bool any = false;
    for (std::uint32_t a = 0; a + 4 <= kDataBytes; a += 4) {
        const std::uint32_t v = cpu_.dataMemory().loadWord(a);
        if (v != 0) { out << "  Mem[" << a << "] = " << s(v) << "\n"; any = true; }
    }
    if (!any) out << "  data memory is all zero\n";
}

void Simulator::printListing(std::ostream& out) const {
    for (std::size_t i = 0; i < program_.words.size(); ++i)
        out << std::setw(4) << i * 4 << ": " << hex32(program_.words[i]) << "  " << program_.source[i] << "\n";
}

} 

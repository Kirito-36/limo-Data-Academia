#include "Pipeline.h"

#include "ALU.h"

namespace limo {

Pipeline::Pipeline(std::uint32_t entryPc)
    : registers_(entryPc), pc_(entryPc) {
}

void Pipeline::clearPipeline() {
    ifId_ = IfId{};
    idEx_ = IdEx{};
    exMem_ = ExMem{};
    memWb_ = MemWb{};
    cycles_ = 0u;
    retired_ = 0u;
    illegal_ = 0u;
}

void Pipeline::reset() {
    registers_.reset();
    memory_.reset();
    pc_ = 0u;
    programEnd_ = 0xFFFFFFFFu;
    clearPipeline();
}

void Pipeline::loadProgram(const std::vector<std::uint8_t>& programBytes, std::uint32_t startAddress) {
    memory_.writeRange(startAddress, programBytes);
    registers_.setPc(startAddress);
    pc_ = startAddress;
    programEnd_ = startAddress + static_cast<std::uint32_t>(programBytes.size());
    clearPipeline();
}

void Pipeline::writeProgramWord(std::uint32_t address, std::uint32_t word) {
    memory_.storeWord(address, word);
}

void Pipeline::setProgramEnd(std::uint32_t endAddress) {
    programEnd_ = endAddress;
}

RegisterFile& Pipeline::registers() { return registers_; }
Memory& Pipeline::memory() { return memory_; }
std::uint32_t Pipeline::pc() const { return pc_; }

const IfId& Pipeline::ifId() const { return ifId_; }
const IdEx& Pipeline::idEx() const { return idEx_; }
const ExMem& Pipeline::exMem() const { return exMem_; }
const MemWb& Pipeline::memWb() const { return memWb_; }

std::uint64_t Pipeline::cycles() const { return cycles_; }
std::uint64_t Pipeline::retired() const { return retired_; }
std::uint64_t Pipeline::illegalCount() const { return illegal_; }

bool Pipeline::empty() const {
    return !ifId_.valid && !idEx_.valid && !exMem_.valid && !memWb_.valid;
}

// ---------------------------------------------------------------- stages

void Pipeline::stageWriteBack(const MemWb& in) {
    if (!in.valid) {
        return;
    }
    ++retired_;
    if (in.wb.regWrite) {
        registers_.write(in.rd, in.wb.memToReg ? in.memData : in.aluResult);
    }
}

MemWb Pipeline::stageMemory(const ExMem& in) {
    MemWb out;
    if (!in.valid) {
        return out;
    }
    out.valid = true;
    out.pc = in.pc;
    out.instr = in.instr;
    out.op = in.op;
    out.aluResult = in.aluResult;
    out.rd = in.rd;
    out.wb = in.wb;

    // Address bits [1:0] are ignored (doc section 1.4).
    const std::uint32_t address = in.aluResult & ~3u;
    if (in.mem.memRead) {
        out.memData = memory_.loadWord(address);
    }
    if (in.mem.memWrite) {
        memory_.storeWord(address, in.storeData);
    }
    return out;
}

ExMem Pipeline::stageExecute(const IdEx& in, bool& redirect, std::uint32_t& target) const {
    redirect = false;
    target = 0u;

    ExMem out;
    if (!in.valid) {
        return out;
    }
    out.valid = true;
    out.pc = in.pc;
    out.instr = in.instr;
    out.op = in.op;
    out.rd = in.rd;
    out.mem = in.mem;
    out.wb = in.wb;

    // ALU_Control = {funct7[5]_eff, funct3}, which maps straight onto ALU::executeR.
    const std::uint32_t a = in.rs1Val;
    const std::uint32_t b = in.ex.aluSrc ? static_cast<std::uint32_t>(in.imm) : in.rs2Val;
    const std::uint8_t funct3 = static_cast<std::uint8_t>(in.ex.aluControl & 0x7u);
    const std::uint8_t funct7 = (in.ex.aluControl & 0x8u) != 0u ? 0x20u : 0x00u;
    const std::uint32_t result = ALU::executeR(funct3, funct7, a, b);

    out.aluResult = result;
    out.storeData = in.rs2Val;

    if (in.ex.branch) {
        // Result is A - B; Zero decides. A separate adder gives the target.
        const bool zero = (result == 0u);
        const bool take = in.ex.branchOnNotZero ? !zero : zero;
        if (take) {
            redirect = true;
            target = in.pc + static_cast<std::uint32_t>(in.imm);
        }
    }
    return out;
}

IdEx Pipeline::stageDecode(const IfId& in) {
    IdEx out;
    if (!in.valid) {
        return out;
    }
    const DecodedControl d = ControlUnit::decode(in.instr);
    if (!d.legal) {
        ++illegal_;
    }

    out.valid = true;
    out.pc = in.pc;
    out.instr = in.instr;
    out.op = d.op;
    out.rs1 = d.rs1;
    out.rs2 = d.rs2;
    out.rd = d.rd;
    out.rs1Val = registers_.read(d.rs1);
    out.rs2Val = registers_.read(d.rs2);
    out.imm = d.imm;
    out.ex = d.ex;
    out.mem = d.mem;
    out.wb = d.wb;
    return out;
}

IfId Pipeline::stageFetch() {
    IfId out;
    if (pc_ >= programEnd_) {
        return out;  // nothing left to fetch: bubble, PC holds
    }
    out.valid = true;
    out.pc = pc_;
    out.instr = memory_.loadWord(pc_);
    pc_ += 4u;
    return out;
}

// ------------------------------------------------------------------ tick

void Pipeline::tick() {
    stageWriteBack(memWb_);
    const MemWb memWbNext = stageMemory(exMem_);

    bool redirect = false;
    std::uint32_t target = 0u;
    const ExMem exMemNext = stageExecute(idEx_, redirect, target);

    IdEx idExNext;
    IfId ifIdNext;
    if (redirect) {
        // Taken branch: the instruction in ID and the one being fetched are on
        // the wrong path. Leave them as bubbles and restart fetch at the target.
        pc_ = target;
    } else {
        idExNext = stageDecode(ifId_);
        ifIdNext = stageFetch();
    }

    // Latch all pipeline registers together.
    memWb_ = memWbNext;
    exMem_ = exMemNext;
    idEx_ = idExNext;
    ifId_ = ifIdNext;
    ++cycles_;
}

std::uint64_t Pipeline::run(std::uint64_t maxCycles) {
    std::uint64_t ran = 0u;
    while (ran < maxCycles && !(pc_ >= programEnd_ && empty())) {
        tick();
        ++ran;
    }
    return ran;
}

}  // namespace limo

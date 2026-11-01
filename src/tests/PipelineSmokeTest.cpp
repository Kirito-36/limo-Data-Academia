#include "ControlUnit.h"
#include "Pipeline.h"

#include <cstdint>
#include <iostream>

using namespace limo;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

// Tiny encoders following docs/CONTROL_UNIT_AND_ALU_LIMO.md (4-bit registers).
static std::uint32_t encR(unsigned f7, unsigned rs2, unsigned rs1, unsigned f3, unsigned rd) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | 0x33u;
}
static std::uint32_t encI(int imm, unsigned rs1, unsigned f3, unsigned rd, unsigned opcode) {
    return ((static_cast<std::uint32_t>(imm) & 0xFFFu) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | opcode;
}
static std::uint32_t encS(int imm, unsigned rs2, unsigned rs1) {
    const std::uint32_t i = static_cast<std::uint32_t>(imm);
    return (((i >> 5) & 0x7Fu) << 25) | (rs2 << 20) | (rs1 << 15) | (2u << 12) | ((i & 0x1Fu) << 7) | 0x23u;
}
static std::uint32_t encB(int imm, unsigned rs2, unsigned rs1, unsigned f3) {
    const std::uint32_t i = static_cast<std::uint32_t>(imm);
    return (((i >> 12) & 1u) << 31) | (((i >> 5) & 0x3Fu) << 25) | (rs2 << 20) | (rs1 << 15) |
           (f3 << 12) | (((i >> 1) & 0xFu) << 8) | (((i >> 11) & 1u) << 7) | 0x63u;
}
static std::uint32_t kopanya(unsigned rd, unsigned rs1, unsigned rs2) { return encR(0x00, rs2, rs1, 0, rd); }
static std::uint32_t tlosa(unsigned rd, unsigned rs1, unsigned rs2)   { return encR(0x20, rs2, rs1, 0, rd); }
static std::uint32_t kapa(unsigned rd, unsigned rs1, unsigned rs2)    { return encR(0x00, rs2, rs1, 6, rd); }
static std::uint32_t le(unsigned rd, unsigned rs1, unsigned rs2)      { return encR(0x00, rs2, rs1, 7, rd); }
static std::uint32_t eketsa(unsigned rd, unsigned rs1, int imm)       { return encI(imm, rs1, 0, rd, 0x13u); }
static std::uint32_t suthisa(unsigned rd, unsigned rs1, int shamt)    { return encI(shamt, rs1, 1, rd, 0x13u); }
static std::uint32_t bala(unsigned rd, int off, unsigned rs1)         { return encI(off, rs1, 2, rd, 0x03u); }
static std::uint32_t beha(unsigned rs2, int off, unsigned rs1)        { return encS(off, rs2, rs1); }
static std::uint32_t lekanya(unsigned rs1, unsigned rs2, int off)     { return encB(off, rs2, rs1, 0); }
static std::uint32_t phapanyo(unsigned rs1, unsigned rs2, int off)    { return encB(off, rs2, rs1, 1); }

static void load(Pipeline& p, const std::vector<std::uint32_t>& words) {
    for (std::size_t i = 0; i < words.size(); ++i) {
        p.writeProgramWord(static_cast<std::uint32_t>(i * 4u), words[i]);
    }
    p.setProgramEnd(static_cast<std::uint32_t>(words.size() * 4u));
}

// --- 1. control unit against the verification table (doc section 6) --------
static void testControlUnit() {
    auto d = ControlUnit::decode(0x002081B3u);  // kopanya r3, r1, r2
    CHECK(d.op == Op::KOPANYA && d.ex.aluOp == 2 && d.ex.aluControl == alu_ctl::ADD && d.wb.regWrite);
    CHECK(d.rd == 3 && d.rs1 == 1 && d.rs2 == 2);
    CHECK(ControlUnit::decode(0x402081B3u).ex.aluControl == alu_ctl::SUB);  // tlosa
    CHECK(ControlUnit::decode(0x0020E1B3u).ex.aluControl == alu_ctl::OR);   // kapa
    CHECK(ControlUnit::decode(0x0020F1B3u).ex.aluControl == alu_ctl::AND);  // le

    d = ControlUnit::decode(0x00508193u);  // eketsa r3, r1, 5
    CHECK(d.op == Op::EKETSA && d.ex.aluOp == 3 && d.ex.aluSrc && d.ex.aluControl == alu_ctl::ADD && d.imm == 5);
    d = ControlUnit::decode(0xC0008193u);  // eketsa r3, r1, -1024: ADD, not SUB
    CHECK(d.op == Op::EKETSA && d.ex.aluControl == alu_ctl::ADD && d.imm == -1024);
    d = ControlUnit::decode(0x00209193u);  // suthisa r3, r1, 2
    CHECK(d.op == Op::SUTHISA && d.ex.aluControl == alu_ctl::SLL && d.imm == 2);

    d = ControlUnit::decode(0x00422183u);  // bala r3, 4(r4)
    CHECK(d.op == Op::BALA && d.ex.aluOp == 0 && d.mem.memRead && d.wb.memToReg && d.wb.regWrite && d.imm == 4);
    d = ControlUnit::decode(0x00322423u);  // beha r3, 8(r4)
    CHECK(d.op == Op::BEHA && d.mem.memWrite && !d.wb.regWrite && d.imm == 8);

    d = ControlUnit::decode(0x00418463u);  // lekanya r3, r4, +8
    CHECK(d.op == Op::LEKANYA && d.ex.branch && !d.ex.branchOnNotZero && d.ex.aluControl == alu_ctl::SUB && d.imm == 8);
    CHECK(ControlUnit::decode(0xFE418CE3u).imm == -8);  // lekanya r3, r4, -8
    d = ControlUnit::decode(0x00419463u);  // phapanyo r3, r4, +8
    CHECK(d.op == Op::PHAPANYO && d.ex.branch && d.ex.branchOnNotZero);

    // NOPs: reserved bit set, and jal (not in the ISA)
    for (std::uint32_t w : {0x002881B3u, 0x0000006Fu}) {
        d = ControlUnit::decode(w);
        CHECK(d.op == Op::NOP && !d.legal && !d.wb.regWrite && !d.mem.memRead && !d.mem.memWrite && !d.ex.branch);
    }
}

// --- 2. one instruction, checked in every pipeline register each cycle -----
static void testSingleInstructionThroughFiveStages() {
    Pipeline p;
    p.registers().write(1, 10);
    load(p, {eketsa(3, 1, 5)});

    p.tick();  // IF
    CHECK(p.ifId().valid && p.ifId().instr == eketsa(3, 1, 5) && p.ifId().pc == 0);
    CHECK(!p.idEx().valid && !p.exMem().valid && !p.memWb().valid);

    p.tick();  // ID
    CHECK(!p.ifId().valid && p.idEx().valid && p.idEx().op == Op::EKETSA);
    CHECK(p.idEx().rs1Val == 10 && p.idEx().imm == 5 && p.idEx().ex.aluSrc && p.idEx().wb.regWrite);

    p.tick();  // EX
    CHECK(!p.idEx().valid && p.exMem().valid && p.exMem().aluResult == 15 && p.exMem().rd == 3);

    p.tick();  // MEM
    CHECK(!p.exMem().valid && p.memWb().valid && p.memWb().aluResult == 15);
    CHECK(p.registers().read(3) == 0);  // not written back yet

    p.tick();  // WB
    CHECK(p.registers().read(3) == 15);
    CHECK(p.retired() == 1 && p.empty());
}

// --- 3. all ALU instructions, independent so no hazard unit is needed ------
static void testAluProgram() {
    Pipeline p;
    p.registers().write(1, 10);
    p.registers().write(2, 20);
    load(p, {kopanya(3, 1, 2), tlosa(4, 2, 1), kapa(5, 1, 2), le(6, 1, 2), suthisa(7, 1, 2), eketsa(8, 1, -1)});
    const std::uint64_t cycles = p.run();
    CHECK(cycles == 6 + 4);  // N instructions take N + 4 cycles
    CHECK(p.registers().read(3) == 30);
    CHECK(p.registers().read(4) == 10);
    CHECK(p.registers().read(5) == (10u | 20u));
    CHECK(p.registers().read(6) == (10u & 20u));
    CHECK(p.registers().read(7) == 40);
    CHECK(p.registers().read(8) == 9);
    CHECK(p.retired() == 6 && p.illegalCount() == 0);
}

// --- 4. store then load through MEM ---------------------------------------
static void testLoadStore() {
    Pipeline p;
    p.registers().write(4, 0x1000);
    p.registers().write(2, 0xCAFEBABEu);
    load(p, {beha(2, 8, 4), bala(5, 8, 4)});
    p.run();
    CHECK(p.memory().loadWord(0x1008) == 0xCAFEBABEu);
    CHECK(p.registers().read(5) == 0xCAFEBABEu);
}

// --- 5. branches: resolved in EX, wrong-path instructions flushed ----------
static void testBranches() {
    {   // lekanya taken (r1 == r2): skips +12, flushes the two behind it
        Pipeline p;
        p.registers().write(1, 7);
        p.registers().write(2, 7);
        load(p, {lekanya(1, 2, 12), eketsa(5, 0, 99), eketsa(6, 0, 99), eketsa(7, 0, 42)});
        p.run();
        CHECK(p.registers().read(5) == 0);
        CHECK(p.registers().read(6) == 0);
        CHECK(p.registers().read(7) == 42);
    }
    {   // phapanyo not taken (r1 == r2): falls through
        Pipeline p;
        p.registers().write(1, 7);
        p.registers().write(2, 7);
        load(p, {phapanyo(1, 2, 12), eketsa(5, 0, 99), eketsa(6, 0, 98), eketsa(7, 0, 42)});
        p.run();
        CHECK(p.registers().read(5) == 99);
        CHECK(p.registers().read(6) == 98);
        CHECK(p.registers().read(7) == 42);
    }
    {   // phapanyo taken (r1 != r2)
        Pipeline p;
        p.registers().write(1, 7);
        p.registers().write(2, 8);
        load(p, {phapanyo(1, 2, 12), eketsa(5, 0, 99), eketsa(6, 0, 99), eketsa(7, 0, 42)});
        p.run();
        CHECK(p.registers().read(5) == 0 && p.registers().read(6) == 0 && p.registers().read(7) == 42);
    }
}

// --- 6. NOP / bubble / illegal words change nothing -----------------------
static void testNops() {
    Pipeline p;
    p.registers().write(1, 3);
    load(p, {0x0000006Fu /* jal: not in the ISA */, 0x002881B3u /* reserved bit set */, eketsa(2, 1, 1)});
    p.run();
    CHECK(p.illegalCount() == 2);
    CHECK(p.registers().read(2) == 4);
    for (unsigned r = 3; r < 16; ++r) CHECK(p.registers().read(static_cast<std::uint8_t>(r)) == 0);
}

int main() {
    testControlUnit();
    testSingleInstructionThroughFiveStages();
    testAluProgram();
    testLoadStore();
    testBranches();
    testNops();
    if (g_failures == 0) {
        std::cout << "Pipeline tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) failed.\n";
    return 1;
}

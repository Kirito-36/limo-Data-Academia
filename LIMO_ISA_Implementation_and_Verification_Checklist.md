# LIMO ISA Implementation and Verification Checklist

**Project:** CS3520 LIMO — Data Academia  
**Purpose:** Provide a shared checklist for checking that the LIMO ISA specification (M1), the C++ reference implementation, the browser simulator, and the tests describe the same processor.

> **Status note:** This document is a verification plan. A checklist item is not evidence that the feature is already implemented. Mark an item complete only after checking the current code and running an appropriate test.

## 1. Architecture baseline

Use the team's approved M1 specification as the source of truth.

| Property | LIMO design |
|---|---|
| Architecture | 32-bit, load-store, RV32I-derived |
| Instruction width | Fixed 32 bits |
| General-purpose registers | 16 registers: `r0`–`r15` |
| Register index width | 4 bits |
| Memory addressing | Byte-addressed |
| Endianness | Little-endian |
| Word size | 4 bytes |
| Word alignment | Address must be divisible by 4 |
| Pipeline | IF → ID → EX → MEM → WB |
| Instruction formats | R-style, I-style and S-style; branches use the layout documented in M1 |

The PC is a separate special-purpose register, not one of the sixteen general-purpose register indices. Sequential execution advances the PC by four bytes. A taken branch selects the branch target instead.

## 2. Instruction behavior checklist

The mnemonic, operand order, encoding fields and behavior must agree across the M1 document, assembler, decoder, C++ implementation, browser simulator and tests.

| Mnemonic | Expected operation | Verification focus |
|---|---|---|
| `kopanya` | Add two register values | Must perform addition, not a bitwise operation |
| `tlosa` | Subtract the second register value from the first | Check operand order and 32-bit result behavior |
| `eketsa` | Add a sign-extended immediate to a register | Check immediate decoding and sign extension |
| `kapa` | Bitwise OR of two register values | Check bit-by-bit OR result |
| `le` | Bitwise AND of two register values | Check bit-by-bit AND result |
| `suthisa` | Shift a register value left by the encoded immediate amount | Check shift amount extraction |
| `bala` | Load a 32-bit word from memory | Check effective address, alignment and destination register |
| `beha` | Store a 32-bit word to memory | Check effective address, stored value and no GPR write-back |
| `lekanya` | Branch when the two source registers are equal | Check taken and not-taken outcomes |
| `phapanyo` | Branch when the two source registers are unequal | Check taken and not-taken outcomes |

**Important:** This table summarises the M1 instruction intent. For exact field positions, immediate ranges, opcode/function values and Sesotho spelling, consult the approved M1 encoding tables rather than inventing new values here.

## 3. Register-file checks

- [ ] The register file contains exactly sixteen general-purpose registers (`r0` through `r15`).
- [ ] Each general-purpose register stores a 32-bit value.
- [ ] Reading `r0` always returns zero.
- [ ] Writing to `r0` does not change its value.
- [ ] Register indices 0–15 are accepted.
- [ ] Register index 16 and above are rejected or handled according to the project's documented invalid-instruction policy.
- [ ] The PC is stored and updated separately from the GPR array.
- [ ] Register names and aliases match the M1 register table.

## 4. Encoding and decoding checks

- [ ] Every supported instruction encodes to exactly 32 bits.
- [ ] The assembler and decoder agree on opcode and function fields.
- [ ] Four-bit register indices are used consistently for the sixteen-register architecture.
- [ ] Any reserved register-field bit required by M1 is checked and is not silently treated as a fifth register bit.
- [ ] I-style and S-style immediates are reconstructed and sign-extended correctly.
- [ ] Branch immediate bits are reconstructed according to the specific LIMO M1 layout.
- [ ] Invalid opcodes, unsupported function combinations and invalid register fields are detected consistently.
- [ ] Labels resolve to the intended instruction addresses.
- [ ] Any assembler pseudo-instruction is expanded into the documented real instruction sequence.

## 5. Memory checks

- [ ] Memory is byte-addressed.
- [ ] A 32-bit word occupies four consecutive byte addresses.
- [ ] Little-endian byte ordering is used consistently for loads and stores.
- [ ] Word load/store addresses are checked for four-byte alignment.
- [ ] The simulator's allocated memory capacity is documented separately from the architectural address space.
- [ ] Out-of-range accesses follow a documented and consistent simulator policy.
- [ ] A store changes memory but does not write an ordinary destination register.
- [ ] A load writes the value read from memory to its destination register.

## 6. Pipeline checks

The intended pipeline has five stages:

| Stage | Responsibility | Evidence to look for |
|---|---|---|
| IF | Fetch instruction using the PC | PC and fetched instruction are visible |
| ID | Decode instruction and read registers | Decoded fields, operands and controls are available |
| EX | ALU operation, address calculation and branch decision | Correct ALU result and branch outcome |
| MEM | Perform load/store access | Correct memory read or write |
| WB | Write selected result to destination register | Correct destination and write-back value |

### Data and control hazards

- [ ] A dependent instruction receives the correct operand through forwarding, where forwarding is supported.
- [ ] A load-use dependency is detected.
- [ ] The required stall/bubble is inserted for a load-use dependency.
- [ ] A taken branch invalidates younger wrong-path instructions according to the documented branch-resolution stage and flush policy.
- [ ] Flushed instructions do not update registers or memory.
- [ ] Pipeline stage state and valid/bubble status are visible in the simulator.
- [ ] Cycle counts, retired instruction counts, stalls and flushes are calculated consistently.

Do not mark forwarding, stalls or flushing complete based only on a design diagram. Confirm the implementation and demonstrate the behavior with a test program.

## 7. Suggested focused test scenarios

These are test scenarios, not pre-filled expected results. Derive exact expected values from the M1 rules and the current implementation.

| Test | Program behavior to exercise | What to verify |
|---|---|---|
| T1 — Register arithmetic | Add and subtract known values | Destination values and PC progression |
| T2 — Immediate arithmetic | Add a positive immediate and a negative immediate | Correct sign extension |
| T3 — Logic | OR and AND values with different bit patterns | Exact bitwise results |
| T4 — Shift | Shift a value with known set bits | Correct shift amount and result |
| T5 — Load/store | Store a word, then load it back | Address, byte order and loaded value |
| T6 — Branch equal | Test both equal and unequal operands | Correct taken/not-taken decision |
| T7 — Branch not equal | Test both unequal and equal operands | Correct taken/not-taken decision |
| T8 — Data dependency | Make an instruction consume a previous result | Correct forwarding or safe stall behavior |
| T9 — Load-use dependency | Load a value and immediately consume it | Required stall and correct final result |
| T10 — Taken branch | Place visible instructions after a taken branch | Wrong-path instructions are flushed |
| T11 — Invalid register | Attempt to reference `r16` | Consistent rejection; no silent aliasing |
| T12 — Alignment/range | Attempt an unaligned or unsupported memory access | Documented simulator behavior |

## 8. Cross-implementation consistency review

For each instruction, compare the same small program in:

1. The assembler output.
2. The C++ reference implementation.
3. The browser simulator.

Record:

| Program/test | Assembler encoding | C++ result | Browser result | Match? | Notes |
|---|---|---|---|---|---|
| T1 | To be recorded | To be recorded | To be recorded | Pending | |
| T2 | To be recorded | To be recorded | To be recorded | Pending | |
| T3 | To be recorded | To be recorded | To be recorded | Pending | |
| T4 | To be recorded | To be recorded | To be recorded | Pending | |
| T5 | To be recorded | To be recorded | To be recorded | Pending | |
| T6–T12 | To be recorded | To be recorded | To be recorded | Pending | |

A mismatch should be reported with the smallest program that reproduces it, the expected behavior from M1, the observed behavior, and the relevant file/function.

## 9. Evidence to attach to a completed review

For each item marked complete, keep at least one form of evidence:

- A unit test and its passing output.
- A small assembly program and its final register/memory state.
- A screenshot or recording of the browser simulator.
- A source-code reference identifying the implementation.
- A comparison showing the C++ and browser versions agree.

## 10. Review record

| Field | Record |
|---|---|
| Repository commit/release checked | To be filled in |
| Date checked | To be filled in |
| Reviewer | To be filled in |
| Tests run | To be filled in |
| Known mismatches | To be filled in |
| Follow-up issues | To be filled in |

---

**Contribution note:** This checklist is intended to support the team's implementation review and testing. It does not replace M1, change the ISA, or claim that unchecked features have been implemented.

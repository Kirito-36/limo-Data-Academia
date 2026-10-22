# LIMO Control Unit and ALU

CS3520 LIMO Group Project · Team Data-Academia · companion to the M1 ISA specification (tag m1). Mnemonics and register names are the Sesotho ones from the ISA; the RV32I equivalent is given in brackets where it helps.

## 1. Instruction decode

### **1.1 Fields**

| **Field** | **Bits** | **Used by** |
| :-: | :-: | :-: |
| opcode | inst\[6:0\] | all |
| rd (4-bit) | inst\[10:7\] | R, I |
| funct3 | inst\[14:12\] | all |
| rs1 (4-bit) | inst\[18:15\] | R, I, S, B |
| rs2 (4-bit) | inst\[23:20\] | R, S, B |
| funct7 | inst\[31:25\] (funct7\[5\] = inst\[30\]) | R, I-shift |

Only the 4-bit fields go to the register file. The reserved top bits are inst\[19\] (rs1), inst\[24\] (rs2) and inst\[11\] (rd).

### **1.2 Which reserved bits exist in which format**

| **Bit** | **Reserved (must be 0) in** | **Carries something else in** |
| :-: | :-: | :-: |
| inst\[19\] (rs1\[4\]) | R, I, S, B | – |
| inst\[24\] (rs2\[4\]) | R, S, B | I and I-shift: imm\[4\] / shamt\[4\] (a valid bit, e.g. *suthisa r3, r1, 16*) |
| inst\[11\] (rd\[4\]) | R, I (including bala) | S: imm\[4\]; B: imm\[11\] |


### **1.3 Legal instruction patterns**

| **Instruction** | **Fmt** | **opcode** | **funct3** | **funct7** |
| :-: | :-: | :-: | :-: | :-: |
| kopanya | R | 0110011 | 000 | 0000000 |
| tlosa | R | 0110011 | 000 | 0100000 |
| kapa | R | 0110011 | 110 | 0000000 |
| le | R | 0110011 | 111 | 0000000 |
| eketsa | I | 0010011 | 000 | – |
| suthisa | I-shift | 0010011 | 001 | 0000000 |
| bala | I | 0000011 | 010 | – |
| beha | S | 0100011 | 010 | – |
| lekanya | B | 1100011 | 000 | – |
| phapanyo | B | 1100011 | 001 | – |

### **1.4 Behaviour outside the instruction set (implementation choice)**

The ISA leaves non-zero reserved bits and misaligned addresses *undefined* and has no exceptions. To make simulation and testing deterministic, our hardware defines them as follows (this is allowed by "undefined" and can be recorded as a design-decision log entry):

- Any instruction that is not one of the 10 patterns in 1.3, or has a reserved bit from 1.2 set, is decoded as a **NOP**: RegWrite = MemWrite = MemRead = Branch = 0. An optional debug output illegal is raised. 

- Data-memory address bits \[1:0\] are ignored (the access is to the word containing the address). 

### **1.5 Immediate generator**

| **ImmSel** | **Used by** | **Immediate (sign-extended from inst\[31\])** |
| :-: | :-: | :-: |
| 00 (I) | eketsa, bala; suthisa uses imm\[4:0\] as shamt | inst\[31:20\] |
| 01 (S) | beha | \{inst\[31:25\], inst\[11:7\]\} |
| 10 (B) | lekanya, phapanyo | \{inst\[31\], inst\[7\], inst\[30:25\], inst\[11:8\], 1'b0\} |

Branch offsets are multiples of 4 in range −4096..+4092; the assembler enforces this, so imm\_B\[1:0\] = 00 for assembled code.

## **2. Main control unit**

| **Instructions** | **Opcode** | **ALUSrc** | **MemToReg** | **RegWrite** | **MemRead** | **MemWrite** | **Branch** | **ALUOp** | **ImmSel** |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| **R-type:** kopanya, tlosa, kapa, le | 0110011 | 0 | 0 | 1 | 0 | 0 | 0 | 10 | 00 |
| **I-type arithmetic:** eketsa, suthisa | 0010011 | 1 | 0 | 1 | 0 | 0 | 0 | **11** | 00 |
| **I-type load:** bala | 0000011 | 1 | 1 | 1 | 1 | 0 | 0 | 00 | 00 |
| **S-type:** beha | 0100011 | 1 | 0 | 0 | 0 | 1 | 0 | 00 | 01 |
| **B-type:** lekanya, phapanyo | 1100011 | 0 | 0 | 0 | 0 | 0 | 1 | 01 | 10 |
| **Anything else (NOP)** | other | 0 | 0 | 0 | 0 | 0 | 0 | 00 | 00 |

- eketsa and suthisa share opcode 0010011, so they get the same main-control row; the ALU control separates them by funct3. 

- I-type arithmetic has its own ALUOp (11) so that **funct7\[5\] = inst\[30\] is ignored for eketsa**: for eketsa, inst\[30\] is just bit 10 of the immediate. Otherwise *eketsa r3, r1, −1024* (0xC0008193) would run as tlosa. 

- For beha the ALU's second operand is the immediate (ALUSrc = 1), while the register value being stored (rs2) goes straight to data memory, after forwarding. 

- Don't-care entries are set to 0 so no X values appear in simulation. 

### **2.1 Signals per pipeline stage**

| **Stage that uses it** | **Signals (carried in ID/EX, EX/MEM, MEM/WB as needed)** |
| :-: | :-: |
| EX | ALUSrc, ALUOp → ALU\_Control, Branch, funct3\[0\] (lekanya / phapanyo select) |
| MEM | MemRead, MemWrite |
| WB | RegWrite, MemToReg |

- **Branch and PC.** Resolved in EX: take = funct3\[0\] ? ~Zero : Zero and PCSrc = Branch & take. When PCSrc = 1 the PC becomes (branch's own address) + imm\_B and the two instructions behind the branch (in IF and ID) are flushed. The target comes from a separate adder in EX, because the ALU is busy subtracting for the comparison. Otherwise the PC becomes PC + 4. There is no link register write, since jal/jalr do not exist. 

- **Flush and stall (interface to the hazard unit).** A flush or a load-use stall bubble is made by forcing RegWrite, MemRead, MemWrite and Branch to 0 in the affected pipeline register. On a load-use stall the PC and IF/ID are held for one cycle. 

- **Forwarding.** The forwarding muxes sit in front of the ALUSrc mux. Their output is also the branch comparison input (rs1 and rs2 in EX) and the store data for beha. 

## **3. ALU design (32-bit)**

The ALU is purely combinational. Its outputs are a 32-bit **Result** and one status signal, **Zero**.

- **Operand A:** Read Data 1 (rs1, after forwarding). 

- **Operand B:** a 2-to-1 mux (ALUSrc) between Read Data 2 (R-type and B-type, after forwarding) and the immediate (eketsa, suthisa, bala, beha). 

- **R-type and I-type arithmetic:** Result goes to the register file (rd). **bala / beha:** Result is the effective address, base + sign-extended offset, sent to data memory. **lekanya / phapanyo:** Result is A − B; only Zero is used. 

- **ADD / SUB:** one adder computing A + (B XOR \{32\{sub\}\}) + sub. 

- **OR / AND:** bitwise. 

- **SLL (suthisa):** left barrel shifter, 5 stages, shamt = B\[4:0\]; zeros are shifted in. 

### **3.1 ALU operations and encoding**

ALU\_Control is 4 bits, formed as \{funct7\[5\]\_eff, funct3\}, so it follows the RV32I encoding and needs almost no decoding. Only five codes are used; the rest are reserved for future extension.

| **Operation** | **ALU\_Control** | **Instructions** |
| :-: | :-: | :-: |
| ADD | 0000 | kopanya, eketsa, bala, beha |
| SLL | 0001 | suthisa |
| OR | 0110 | kapa |
| AND | 0111 | le |
| SUB | 1000 | tlosa, lekanya, phapanyo |

## **4. ALU control unit**

| **ALUOp** | **Category** | **Instruction** | **funct3** | **funct7\[5\] (inst\[30\])** | **Operation** | **ALU\_Control** |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| 00 | Load / store | bala, beha | 010 (memory size; not used by the ALU) | x | ADD | 0000 |
| 01 | Branch | lekanya, phapanyo | 000 / 001 | x | SUB | 1000 |
| 10 | R-type | kopanya | 000 | 0 | ADD | 0000 |
| 10 | R-type | tlosa | 000 | 1 | SUB | 1000 |
| 10 | R-type | kapa | 110 | 0 | OR | 0110 |
| 10 | R-type | le | 111 | 0 | AND | 0111 |
| 11 | I-type arithmetic | eketsa | 000 | **ignored** (imm\[10\]) | ADD | 0000 |
| 11 | I-shift | suthisa | 001 | 0 | SLL | 0001 |

Equivalent logic:

```
`f7b5\_eff = (ALUOp == 10 && funct3 == 000) ? funct7\_5 : 0;`


`case (ALUOp)`

`  00:      ALU\_Control = 0000;                 // ADD  (bala, beha)`

`  01:      ALU\_Control = 1000;                 // SUB  (lekanya, phapanyo)`

`  default: ALU\_Control = \{f7b5\_eff, funct3\};   // 10 = R-type, 11 = I-type`

endcase
```

Because funct3 only distinguishes the five LIMO operations, the 'default' branch gives exactly the codes in 3.1 for legal instructions. Illegal patterns never reach it with RegWrite = 1 (see 1.4).

## **5. ALU status (Zero only)**

| **Signal** | **Logic** | **Use** |
| :-: | :-: | :-: |
| **Zero** | 1 if Result == 0 (taken from the A − B result when ALU\_Control = SUB) | lekanya: branch if Zero = 1. phapanyo: branch if Zero = 0. |

- LIMO has no flags register and only equality branches, so Negative, Carry and Overflow are **not needed**; Zero is an internal ALU-to-branch-logic wire, not architectural state. 

- If slt/sltu or blt/bge are added in a later version, add N, C and V then (SLT = N XOR V, SLTU = NOT C, with C = 1 meaning no borrow). 

- Arithmetic overflow is ignored: results wrap modulo 232 and there are no exceptions. 

## **6. Verification tests**

Encodings use the ISA's own worked examples where they exist.

| **Instruction** | **Hex** | **Expected** |
| :-: | :-: | :-: |
| kopanya r3, r1, r2 | 0x002081B3 | ALUOp 10, ALU\_Control 0000, RegWrite 1 |
| tlosa r3, r1, r2 | 0x402081B3 | ALU\_Control 1000 |
| kapa r3, r1, r2 | 0x0020E1B3 | ALU\_Control 0110 |
| le r3, r1, r2 | 0x0020F1B3 | ALU\_Control 0111 |
| eketsa r3, r1, 5 | 0x00508193 | ALUOp 11, ALUSrc 1, ALU\_Control 0000, imm = 5 |
| eketsa r3, r1, −1024 | 0xC0008193 | **ADD** (0000), not SUB, although inst\[30\] = 1 |
| suthisa r3, r1, 2 | 0x00209193 | ALU\_Control 0001, shamt = 2 |
| bala r3, 4(r4) | 0x00422183 | ALUOp 00, MemRead 1, MemToReg 1, RegWrite 1, imm = 4 |
| beha r3, 8(r4) | 0x00322423 | ALUOp 00, MemWrite 1, RegWrite 0, ImmSel 01, imm = 8 |
| lekanya r3, r4, +8 | 0x00418463 | ALU\_Control 1000, Branch 1, take = Zero, imm\_B = +8 |
| lekanya r3, r4, −8 | 0xFE418CE3 | imm\_B = −8 (negative offset sign-extension) |
| phapanyo r3, r4, +8 | 0x00419463 | Branch 1, take = NOT Zero |
| kopanya with rs1\[4\] = 1 | 0x002881B3 | NOP (all write/branch signals 0) |
| jal (opcode 1101111) | 0x0000006F | NOP |


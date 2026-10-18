#include "RegisterFile.h"
#include "Memory.h"

#include <cassert>
#include <iostream>

int main() {
    limo::RegisterFile registerFile(0x100u);
    assert(registerFile.pc() == 0x100u);

    registerFile.write(0u, 0xDEADBEEF);
    assert(registerFile.read(0u) == 0u);

    registerFile.write(1u, 0x11223344u);
    assert(registerFile.read(1u) == 0x11223344u);

    registerFile.setPc(0x200u);
    assert(registerFile.pc() == 0x200u);

    limo::Memory memory;
    memory.storeWord(0x1000u, 0xAABBCCDDu);
    assert(memory.loadWord(0x1000u) == 0xAABBCCDDu);

    memory.storeByte(0x2000u, 0x11u);
    memory.storeByte(0x2001u, 0x22u);
    memory.storeByte(0x2002u, 0x33u);
    memory.storeByte(0x2003u, 0x44u);
    assert(memory.loadWord(0x2000u) == 0x44332211u);

    try {
        memory.loadHalfWord(0x2001u);
        assert(false && "Misaligned half-word access should have failed.");
    } catch (const std::invalid_argument&) {
    }

    std::cout << "Register file and memory tests passed.\n";
    return 0;
}

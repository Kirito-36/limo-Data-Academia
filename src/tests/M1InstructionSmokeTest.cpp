#include "CPU.h"
#include "InstructionDecoder.h"

#include <cassert>
#include <iostream>

int main() {
    const std::uint32_t addWord = 0x002081B3u; // add x3, x1, x2
    const auto decoded = limo::InstructionDecoder::decode(addWord);
    assert(decoded.valid);
    assert(decoded.type == limo::Type::R);

    limo::CPU cpu(0x100u);
    cpu.registers().write(1u, 10u);
    cpu.registers().write(2u, 20u);
    cpu.memory().storeWord(0x100u, addWord);

    const bool stepped = cpu.step();
    assert(stepped);
    assert(cpu.pc() == 0x104u);
    assert(cpu.registers().read(3u) == 30u);

    std::cout << "M1 instruction smoke test passed.\n";
    return 0;
}

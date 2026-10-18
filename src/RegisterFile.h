#pragma once

#include "cpp.h"

namespace limo {

class RegisterFile {
public:
    explicit RegisterFile(std::uint32_t initialPc = 0u);

    void reset();

    std::uint32_t read(std::uint8_t reg) const;
    void write(std::uint8_t reg, std::uint32_t value);

    void setPc(std::uint32_t pc);
    std::uint32_t pc() const;

private:
    std::array<std::uint32_t, detail::kRegisterCount> registers_{};
    std::uint32_t pc_ = 0u;
};

}  // namespace limo

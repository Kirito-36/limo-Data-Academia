cmake_minimum_required(VERSION 3.15)
project(limo-Data-Academia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Source include dir
include_directories(${PROJECT_SOURCE_DIR}/src)

# Core library
set(LIMO_SOURCES
  src/RegisterFile.cpp
  src/Memory.cpp
  src/InstructionDecoder.cpp
  src/ALU.cpp
  src/CPU.cpp
)
add_library(limo STATIC ${LIMO_SOURCES})
target_include_directories(limo PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_compile_features(limo PUBLIC cxx_std_17)

# Tests / tools
add_executable(register_memory_test src/tests/RegisterMemorySmokeTest.cpp)
target_link_libraries(register_memory_test PRIVATE limo)

add_executable(m1_instruction_test src/tests/M1InstructionSmokeTest.cpp)
target_link_libraries(m1_instruction_test PRIVATE limo)

# Optional: an instruction execution smoke test if present
if(EXISTS "${PROJECT_SOURCE_DIR}/src/tests/InstructionExecutionSmokeTest.cpp")
  add_executable(instr_exec_test src/tests/InstructionExecutionSmokeTest.cpp)
  target_link_libraries(instr_exec_test PRIVATE limo)
  add_test(NAME instr_exec_test COMMAND instr_exec_test)
endif()

enable_testing()
add_test(NAME register_memory_test COMMAND register_memory_test)
add_test(NAME m1_instruction_test COMMAND m1_instruction_test)

# Install rules (optional)
install(TARGETS limo
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# Quick build instructions
message(STATUS "To build: mkdir -p build && cd build && cmake .. && cmake --build . -- -j")

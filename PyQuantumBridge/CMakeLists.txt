cmake_minimum_required(VERSION 3.15)
project(quantum_bridge CXX)

# Require x64 — QuantumLib.dll is only distributed as Win32/x64, not ARM.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only x64 builds are supported. Run cmake with -A x64.")
endif()

find_package(Python REQUIRED COMPONENTS Development.Module)
find_package(pybind11 CONFIG REQUIRED)

set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(QUANTUM_INCLUDE "${SDK_ROOT}/QuantumLib/QuantumLib/Include")
set(QUANTUM_LIB     "${SDK_ROOT}/Output/Release_x64/QuantumLib.lib")
set(QUANTUM_DLL     "${SDK_ROOT}/Output/Release_x64/QuantumLib.dll")

if(NOT EXISTS "${QUANTUM_LIB}")
    message(FATAL_ERROR
        "QuantumLib.lib not found at ${QUANTUM_LIB}\n"
        "Build the QuantumLib project in Visual Studio (Release x64) first.")
endif()

pybind11_add_module(quantum_bridge src/quantum_bridge.cpp)

target_include_directories(quantum_bridge PRIVATE "${QUANTUM_INCLUDE}")
target_link_libraries(quantum_bridge PRIVATE "${QUANTUM_LIB}")

# Silence MSVC warning C4251 (DLL-interface for STL members in exported class)
target_compile_options(quantum_bridge PRIVATE /wd4251)

# Copy QuantumLib.dll next to the built .pyd so Python finds it at import time
add_custom_command(TARGET quantum_bridge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QUANTUM_DLL}"
        "$<TARGET_FILE_DIR:quantum_bridge>/QuantumLib.dll"
    COMMENT "Copying QuantumLib.dll alongside quantum_bridge.pyd")

message(STATUS "quantum_bridge will link against: ${QUANTUM_LIB}")

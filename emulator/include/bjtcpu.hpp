#pragma once

#include <stdint.h>
#include <array>

#include "opcodes.hpp"

class bjtcpu {
public:
    bjtcpu();

    void reset();

    void loadROM(uint8_t* bytes, size_t size);

    void step();

private:
    uint8_t getInstrLen(uint8_t opcode);

    bool callFuncStep(bool funcInAddr);
    bool retFuncStep();

    bool pushStep(uint8_t value);
    bool popStep(uint8_t reg);
    
    void endCycle();

    void updateFlags(uint8_t lastValue, uint8_t value, bool add);

    void writeRAM(uint8_t bank, uint8_t addr, uint8_t value);
    uint8_t readRAM(uint8_t bank, uint8_t addr);

private:
    uint16_t pcReg;
    std::array<uint8_t, 3> instrReg;
    uint8_t instrFetchIdx;
    uint8_t instrStageIdx;
    
    static constexpr bool REG_WRITABLE[0x10] = {
        true, true, true,   // ra, rb, rc
        false, false, false, false, false, false,
        true, true, true,   // rdis, rsp, rbp
        false, false,
        true, true          // rbnk, radr
    };
    
    static constexpr bool REG_READABLE[0x10] = {
        true, true, true,   // ra, rb, rc
        false, false, false, false, false, false,
        false, true, true,  // rsp, rbp
        false, false,
        true, true          // rbnk, radr
    };
    
    std::array<uint8_t, 0xF> regFile;

    uint8_t flagsReg;

    std::array<uint8_t, 0x10000> rom;
    std::array<uint8_t, 0x10000> ram;

    bool stopped;

};
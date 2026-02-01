#include "bjtcpu.hpp"

bjtcpu::bjtcpu() {
    rom.fill(0);
    reset();
}

void bjtcpu::reset() {
    pcReg = 0;
    instrReg.fill(0);

    instrFetchIdx = 0;
    instrStageIdx = 0;

    regFile.fill(0);
    flagsReg = 0;

    ram.fill(0);

    stopped = false;
}

void bjtcpu::loadROM(uint8_t* bytes, size_t size) {
    rom.fill(0);
    std::memcpy(rom.data(), bytes, size);
}

void bjtcpu::step() {
    if (stopped) {
        return;
    }

    if (instrFetchIdx == 0 || instrFetchIdx < getInstrLen(instrReg[0])) {
        instrReg[instrFetchIdx] = rom[pcReg];
        instrFetchIdx++;
        pcReg++;

        return;
    }

    uint8_t opcode = instrReg[0];
    uint8_t destReg = instrReg[0] & 0xF;

    switch ((opcode >> 4) & 0xF) {
        case 0x0: {
            if (opcode == 0x00) {
                stopped = true;
                return;
            } else if (opcode == 0x01) {
                
            }
        }
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
    }
}

uint8_t bjtcpu::getInstrLen(uint8_t opcode) {

}

bool bjtcpu::callFuncStep(bool funcInAddr) {

}

bool bjtcpu::retFuncStep() {

}

bool bjtcpu::pushStep(uint8_t value) {

}

bool bjtcpu::popStep(uint8_t reg) {

}
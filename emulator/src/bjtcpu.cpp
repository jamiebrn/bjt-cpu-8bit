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

    bool cycleFinished = true;

    switch ((opcode >> 4) & 0xF) {
        case 0x0: {
            if (opcode == OP_STOP) {
                stopped = true;
                return;

            } else if (opcode == OP_RET) {
                retFuncStep();

            } else if (opcode == OP_PCALL) {
                callFuncStep(true);
            }
            break;
        }
        case 0x1: {
            if (opcode == OP_PUSH) {
                cycleFinished = pushStep(regFile[(instrReg[1] >> 4) & 0xF]);

            } else if (opcode == OP_STO) {
                writeRAM(regFile[REG_BNK], regFile[REG_ADDR], regFile[(instrReg[1] >> 4) & 0xF]);

            } else if (opcode == OP_CMP) {
                uint8_t firstValue = regFile[(instrReg[1] >> 4) & 0xF];
                uint8_t secondValue = regFile[instrReg[1] & 0xF];
                updateFlags(firstValue, firstValue - secondValue, false);
            }
            break;
        }
        case 0x2: {
            cycleFinished = popStep((opcode >> 4) & 0xF);
            break;
        }
        case 0x3: {
            regFile[(opcode >> 4) & 0xF] = rom[regFile[REG_BNK] * 0x100 + regFile[REG_ADDR]];
            break;
        }
        case 0x4:
        case 0x5: {
            uint8_t lastValue = regFile[(opcode >> 4) & 0xF];
            regFile[(opcode >> 4) & 0xF] = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];

            if ((opcode & 0xF0) == OP_ADDC && FLAG_CMASK(flagsReg)) {
                regFile[(opcode >> 4) & 0xF]++;
            }

            updateFlags(lastValue, regFile[(opcode >> 4) & 0xF], true);

            break;
        }
        case 0x6: {
            uint8_t addr = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];
            writeRAM(regFile[REG_BNK], addr, regFile[REG_A]);
            break;
        }
        case 0x7:
        case 0x8: {
            uint8_t lastValue = regFile[(opcode >> 4) & 0xF];
            regFile[(opcode >> 4) & 0xF] = regFile[(instrReg[1] >> 4) & 0xF] - regFile[instrReg[1] & 0xF];
            
            if ((opcode & 0xF0) == OP_SUBC && FLAG_CMASK(flagsReg)) {
                regFile[(opcode >> 4) & 0xF]++;
            }
            
            updateFlags(lastValue, regFile[(opcode >> 4) & 0xF], true);
            
            break;
        }
        case 0x9: {
            uint8_t addr = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];
            regFile[(opcode >> 4) & 0xF] = readRAM(regFile[REG_BNK], addr);
            break;
        }
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
    }

    instrStageIdx++;

    if (cycleFinished) {
        endCycle();
    }
}

uint8_t bjtcpu::getInstrLen(uint8_t opcode) {
    opcode = (opcode & 0xF0) >> 4;

    switch (opcode) {
        case 0x0:
        case 0x2:
        case 0x3:
        case 0xF:
            return 1;
        
        case 0x1:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
            return 2;
        
        case 0xC:
        case 0xD:
        case 0xE:
            return 3;
    }

    return 0;
}

bool bjtcpu::callFuncStep(bool funcInAddr) {

}

bool bjtcpu::retFuncStep() {

}

bool bjtcpu::pushStep(uint8_t value) {
    if (instrStageIdx == 0) {
        writeRAM(0xFF, regFile[REG_SP], value);
        return false;
    } else if (instrStageIdx == 1) {
        regFile[REG_SP]++;
    }

    return true;
}

bool bjtcpu::popStep(uint8_t reg) {
    if (instrStageIdx == 0) {
        regFile[REG_SP]--;
        return false;
    } else if (instrStageIdx == 1) {
        regFile[reg] = readRAM(0xFF, regFile[REG_SP]);
    }

    return true;
}

void bjtcpu::endCycle() {
    instrReg.fill(0);
    instrFetchIdx = 0;
    instrStageIdx = 0;
}

void bjtcpu::updateFlags(uint8_t lastValue, uint8_t value, bool add) {
    flagsReg = 0;

    if (value == 0) {
        flagsReg |= (1 << FLAG_ZBIT);
    }

    if (value > 0x7F) {
        flagsReg |= (1 << FLAG_NBIT);

        if (lastValue <= 0x7F) {
            flagsReg |= (1 << FLAG_OBIT);
        }
    }

    if (add && lastValue > value) {
        flagsReg |= (1 << FLAG_CBIT);
    }
}

void bjtcpu::writeRAM(uint8_t bank, uint8_t addr, uint8_t value) {
    ram[bank * 0x100 + addr] = value;
}

uint8_t bjtcpu::readRAM(uint8_t bank, uint8_t addr) {
    return ram[bank * 0x100 + addr];
}
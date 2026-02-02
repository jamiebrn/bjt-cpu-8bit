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
            cycleFinished = popStep(destReg);
            break;
        }
        case 0x3: {
            regFile[destReg] = rom[regFile[REG_BNK] * 0x100 + regFile[REG_ADDR]];
            break;
        }
        case 0x4:
        case 0x5:
        case 0xC: {
            uint8_t lastValue = regFile[destReg];

            if (opcode == OP_IADD) {
                regFile[destReg] = regFile[(instrReg[1] >> 4) & 0xF] + instrReg[2];
            } else {
                regFile[destReg] = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];
            }

            if ((opcode & 0xF0) == OP_ADDC && FLAG_CMASK(flagsReg)) {
                regFile[destReg]++;
            }

            updateFlags(lastValue, regFile[destReg], true);

            break;
        }
        case 0x6: {
            uint8_t addr = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];
            writeRAM(regFile[REG_BNK], addr, regFile[REG_A]);
            break;
        }
        case 0x7:
        case 0x8:
        case 0xD: {
            uint8_t lastValue = regFile[destReg];
            
            if (opcode == OP_ISUB) {
                regFile[destReg] = regFile[(instrReg[1] >> 4) & 0xF] - instrReg[2];
            } else {
                regFile[destReg] = regFile[(instrReg[1] >> 4) & 0xF] - regFile[instrReg[1] & 0xF];
            }
            
            if ((opcode & 0xF0) == OP_SUBC && FLAG_CMASK(flagsReg)) {
                regFile[destReg]++;
            }
            
            updateFlags(lastValue, regFile[destReg], true);
            
            break;
        }
        case 0x9: {
            uint8_t addr = regFile[(instrReg[1] >> 4) & 0xF] + regFile[instrReg[1] & 0xF];
            regFile[destReg] = readRAM(regFile[REG_BNK], addr);
            break;
        }
        case 0xA: {
            regFile[destReg] = instrReg[1];
            break;
        }
        case 0xB: {
            regFile[destReg] = ~(regFile[(instrReg[1] >> 4) & 0xF] & regFile[instrReg[1] & 0xF]);
            break;
        }
        case 0xE: {
            if (opcode == OP_CALL) {
                cycleFinished = callFuncStep(true);
                break;
            }

            if (opcode == OP_JMP ||
                opcode == OP_JMPZ && FLAG_ZMASK(flagsReg) ||
                opcode == OP_JMPN && FLAG_NMASK(flagsReg) ||
                opcode == OP_JMPC && FLAG_CMASK(flagsReg) ||
                opcode == OP_JMPO && FLAG_OMASK(flagsReg)) {
                pcReg = (instrReg[1] << 8) | instrReg[2];
            }
            
            break;
        }
        case 0xF: {
            regFile[destReg] = readRAM(regFile[REG_BNK], regFile[REG_ADDR]);
            break;
        }
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
    switch (instrStageIdx) {
        case 0:
            writeRAM(0xFF, regFile[REG_SP], regFile[REG_BP]);
            return false;
        case 1:
            regFile[REG_SP]++;
            return false;
        case 2:
            writeRAM(0xFF, regFile[REG_SP], pcReg & 0xFF);
            return false;
        case 3:
            regFile[REG_SP]++;
            return false;
        case 4:
            writeRAM(0xFF, regFile[REG_SP], (pcReg >> 8) & 0xFF);
            return false;
        case 5:
            regFile[REG_SP]++;
            pcReg = (instrReg[1] << 8) | instrReg[2];
            return false;
        case 6:
            regFile[REG_BP] = regFile[REG_SP];
            break;
    }

    return true;
}

bool bjtcpu::retFuncStep() {
    switch (instrStageIdx) {
        case 0:
            regFile[REG_SP] = regFile[REG_BP] - 1;
            return false;
        case 1:
            pcReg = (pcReg & 0xFF) | (readRAM(0xFF, regFile[REG_SP]) << 8);
            return false;
        case 2:
            regFile[REG_SP]--;
            return false;
        case 3:
            pcReg = (pcReg & 0xFF00) | readRAM(0xFF, regFile[REG_SP]);
            return false;
        case 4:
            regFile[REG_SP]--;
            return false;
        case 5:
            regFile[REG_BP] = readRAM(0xFF, regFile[REG_SP]);
            break;
    }
    
    return true;
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

uint8_t bjtcpu::readROM(uint8_t bank, uint8_t addr) {
    return rom[bank * 0x100 + addr];
}

uint8_t bjtcpu::getRegValue(uint8_t reg) {
    return regFile[reg];
}
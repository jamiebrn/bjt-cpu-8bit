#include <cstdint>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>

struct InstrData;

enum class TokenType {
    INSTR,
    REG,
    VALUE,
    LABEL,
    LABEL_DEF
};

struct Token {
    TokenType type;
    std::string str;
    size_t line;
    union {
        const InstrData* instrData = nullptr;
        uint8_t regValue;
    };
};

struct InstrData {
    std::vector<TokenType> tokenSuffixes;
    uint8_t opcode;
    bool embedDestReg = false;
};

std::unordered_map<std::string, InstrData> instructionData = {
    {"stop",    {{}, 0x00}},
    {"ret",     {{}, 0x01}},
    {"pcall",   {{}, 0x02}},
    {"pop",     {{TokenType::REG}, 0x20, true}},
    {"lda",     {{TokenType::REG}, 0xF0, true}},
    {"plda",    {{TokenType::REG}, 0x30, true}},
    {"add",     {{TokenType::REG, TokenType::REG, TokenType::REG}, 0x40, true}},
    {"addc",    {{TokenType::REG, TokenType::REG, TokenType::REG}, 0x50, true}},
    {"sub",     {{TokenType::REG, TokenType::REG, TokenType::REG}, 0x70, true}},
    {"subc",    {{TokenType::REG, TokenType::REG, TokenType::REG}, 0x80, true}},
    {"imm",     {{TokenType::REG, TokenType::VALUE}, 0xA0, true}},
    {"nand",    {{TokenType::REG, TokenType::REG, TokenType::REG}, 0xB0, true}},
    {"push",    {{TokenType::REG}, 0x10}},
    {"sto",     {{TokenType::REG}, 0x11}},
    {"cmp",     {{TokenType::REG, TokenType::REG}, 0x12}},
    {"strla",   {{TokenType::REG, TokenType::REG}, 0x60}},
    {"ldrl",    {{TokenType::REG, TokenType::REG, TokenType::REG}, 0x90, true}},
    {"iadd",    {{TokenType::REG, TokenType::REG, TokenType::VALUE}, 0xC0, true}},
    {"isub",    {{TokenType::REG, TokenType::REG, TokenType::VALUE}, 0xD0, true}},
    {"jmp",     {{TokenType::LABEL}, 0xE0}},
    {"jmpz",    {{TokenType::LABEL}, 0xE1}},
    {"jmpn",    {{TokenType::LABEL}, 0xE2}},
    {"jmpc",    {{TokenType::LABEL}, 0xE4}},
    {"jmpo",    {{TokenType::LABEL}, 0xE8}},
    {"call",    {{TokenType::LABEL}, 0xEA}}
};

std::unordered_map<std::string, uint8_t> registerNames = {
    {"ra",      0x00},
    {"rb",      0x01},
    {"rc",      0x02},
    {"rsp",     0x0A},
    {"rbp",     0x0B},
    {"rbnk",    0x0E},
    {"radr",    0x0F}
};

char charLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }

    return c;
}

bool parseInt(const std::string& str, uint8_t& value) {
    if (str.length() < 1) return false;

    bool negative = false;
    int parseValue = 0;
    for (int i = 0; i < str.length(); i++) {
        char c = str[i];

        if (i == 0 && c == '-') {
            negative = true;
            continue;
        }

        if (c >= '0' && c <= '9') {
            parseValue = parseValue * 10 + (c - '0');
        } else {
            return false;
        }
    }

    if (negative) {
        parseValue = -parseValue;
        if (parseValue > 127 || parseValue < -128) {
            return false;
        }
    } else if (parseValue >= 256) {
        return false;
    }

    value = (int8_t)parseValue;

    return true;
}

bool parseHex(const std::string& str, uint8_t& value) {
    if (str.length() < 3 || str.length() > 4 || str[0] != '0' || charLower(str[1]) != 'x') {
        return false;
    }

    value = 0;
    for (int i = 2; i < str.length(); i++) {
        char c = charLower(str[i]);

        if (c >= '0' && c <= '9') {
            value = (value << 4) + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value = (value << 4) + (c - 'a' + 10);
        } else {
            return false;
        }
    }

    return true;
}

bool parseValue(const std::string& str, uint8_t& value) {
    if (parseInt(str, value)) {
        return true;
    } else if (parseHex(str, value)) {
        return true;
    }

    return false;
}

Token createToken(const std::string& str, size_t line) {
    Token token;
    token.str = str;
    token.line = line;

    if (auto iter = instructionData.find(str); iter != instructionData.end()) {
        token.type = TokenType::INSTR;
        token.instrData = &iter->second;
    } else if (auto iter = registerNames.find(str); iter != registerNames.end()) {
        token.type = TokenType::REG;
        token.regValue = iter->second;
    } else if (str[0] >= '0' && str[0] <= '9') {
        token.type = TokenType::VALUE;
    } else if (str[str.size() - 1] == ':') {
        token.type = TokenType::LABEL_DEF;
    } else {
        token.type = TokenType::LABEL;
    }

    return token;
}

std::vector<Token> tokenise(const std::string& text) {
    std::vector<Token> tokens;
    std::string tokenBuffer;

    size_t line = 0;

    for (char c : text) {
        if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9')
            && c != '.' && c != ':' && c != '[' && c != ']') {
            if (!tokenBuffer.empty()) {
                tokens.push_back(createToken(tokenBuffer, line));
                tokenBuffer.clear();
            }

            if (c == '\n') {
                line++;
            }
            continue;
        }

        tokenBuffer += c;
    }

    return tokens;
}

std::vector<uint8_t> assemble(const std::string& text) {
    static constexpr int HEADER_SIZE = 3;
    static const char* DATA_DIRECTIVE = "[data]";
    static const char* PROGRAM_DIRECTIVE = "[program]";

    std::vector<Token> tokens = tokenise(text);
    std::vector<uint8_t> bytecode(HEADER_SIZE, 0x00);
    bytecode[0] = instructionData.at("jmp").opcode;

    std::unordered_map<std::string, uint16_t> labelDefs;
    std::unordered_map<uint16_t, std::pair<std::string, size_t>> labelRefs; // value: label, line

    for (auto token = tokens.begin(); token != tokens.end(); token++) {
        printf("TOKEN TYPE: %d      %s\n", token->type, token->str.c_str());
    }

    bool programMode = true;

    for (auto token = tokens.begin(); token != tokens.end();) {
        if (token->type == TokenType::LABEL_DEF) {
            if (labelDefs.contains(token->str)) {
                printf("ERROR: Redefinition of label on line %zu: %s\n", token->line, token->str.c_str());
                return {};
            }

            labelDefs[token->str.substr(0, token->str.size() - 1)] = bytecode.size();
            token++;
            continue;
        }

        if (programMode) {
            if (token->str == DATA_DIRECTIVE) {
                programMode = false;
            } else if (token->type == TokenType::INSTR) {
                Token instrToken = *token;
                uint8_t opcode = instrToken.instrData->opcode;
                
                if (instrToken.instrData->embedDestReg) {
                    token++;
                    if (token == tokens.end() || token->type != TokenType::REG) {
                        printf("ERROR: Expected register operand on line %zu for instruction: %s\n", instrToken.line, instrToken.str.c_str());
                        return {};
                    }

                    opcode |= token->regValue;
                }

                bytecode.push_back(opcode);

                uint8_t operand = 0;
                bool pushedReg = false;

                for (int i = instrToken.instrData->embedDestReg ? 1 : 0; i < instrToken.instrData->tokenSuffixes.size(); i++) {
                    token++;
                    if (token == tokens.end()) {
                        printf("ERROR: Expected operand on line %zu for instruction: %s\n", instrToken.line, instrToken.str.c_str());
                        return {};
                    }

                    if (token->type != instrToken.instrData->tokenSuffixes[i]) {
                        printf("ERROR: Unexpected token on line %zu: %s\n", token->line, token->str.c_str());
                        return {};
                    }

                    if (token->type == TokenType::REG) {
                        operand = operand << 4;
                        operand |= token->regValue;
                        if (pushedReg) {
                            bytecode.push_back(operand);
                            operand = 0;
                        }
                        pushedReg = !pushedReg;
                    } else if (token->type == TokenType::VALUE) {
                        if (operand != 0) {
                            bytecode.push_back(operand << 4);
                        }
                        if (parseValue(token->str, operand)) {
                            bytecode.push_back(operand);
                            operand = 0;
                        } else {
                            printf("ERROR: Failed to parse value on line %zu: %s\n", token->line, token->str.c_str());
                            return {};
                        }
                    } else if (token->type == TokenType::LABEL) {
                        labelRefs[bytecode.size()] = {token->str, token->line}; // replace with correct label addr after assembled
                        bytecode.push_back(0x00);
                        bytecode.push_back(0x00);
                    }
                }

                if (operand != 0) {
                    bytecode.push_back(operand << 4);
                }
            } else {
                printf("ERROR: Unexpected stray token on line %zu: %s\n", token->line, token->str.c_str());
                return {};
            }
        } else {
            if (token->str == PROGRAM_DIRECTIVE) {
                programMode = true;
            } else if (token->type != TokenType::VALUE) {
                printf("ERROR: Unexpected label in data section on line %zu: %s\n", token->line, token->str.c_str());
                return {};
            } else {
                uint8_t value;
                if (parseValue(token->str, value)) {
                    bytecode.push_back(value);
                } else {
                    printf("ERROR: Failed to parse value on line %zu: %s\n", token->line, token->str.c_str());
                    return {};
                }
            }
        }

        token++;
    }

    if (auto mainLabel = labelDefs.find("main"); mainLabel != labelDefs.end()) {
        uint16_t mainAddr = mainLabel->second;
        bytecode[1] = (mainAddr >> 8) & 0xFF;
        bytecode[2] = mainAddr & 0xFF;
    }

    for (auto label = labelRefs.begin(); label != labelRefs.end(); label++) {
        uint16_t addr = label->first;
        if (auto labelDef = labelDefs.find(label->second.first); labelDef != labelDefs.end()) {
            bytecode[addr] = (labelDef->second >> 8) & 0xFF;
            bytecode[addr + 1] = labelDef->second & 0xFF;
        } else {
            printf("ERROR: Undefined label on line %zu: %s\n", label->second.second, label->second.first.c_str());
            return {};
        }
    }

    return bytecode;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Must provide asm file\n");
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    std::string fileStr(fileSize, 0);
    file.seekg(0);
    file.read(&fileStr[0], fileSize);
    file.close();

    std::vector<uint8_t> bytecode = assemble(fileStr);

    std::string filename(argv[1]);
    std::ofstream outFile(filename.substr(0, filename.find_last_of('.')) + ".bin", std::ios::binary);
    
    for (uint8_t byte : bytecode) {
        outFile << byte;
    }
    outFile.close();

    return 0;
}
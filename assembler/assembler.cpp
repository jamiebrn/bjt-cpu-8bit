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
    std::unordered_map<uint16_t, std::string> labelRefs;

    bool programMode = true;

    for (auto token = tokens.begin(); token != tokens.end();) {
        if (token->type == TokenType::LABEL_DEF) {
            if (labelDefs.contains(token->str)) {
                printf("ERROR: Redefinition of label on line %zu: %s\n", token->line, token->str.c_str());
                return {};
            }

            labelDefs[token->str] = bytecode.size();
        }

        if (programMode) {
            if (token->str == DATA_DIRECTIVE) {
                programMode = false;
            } else if (token->type == TokenType::INSTR) {
                Token instrToken = *token;
                uint8_t opcode = instrToken.instrData->opcode;
                
                if (instrToken.instrData->embedDestReg) {
                    token++;
                    if (token->type != TokenType::REG) {
                        printf("ERROR: Expected register operand on line %zu: %s\n", token->line, token->str.c_str());
                        return {};
                    }

                    opcode |= token->regValue;
                }

                for (int i = instrToken.instrData->embedDestReg ? 1 : 0; i < instrToken.instrData->tokenSuffixes.size(); i++) {
                    token++;
                    if (token->type != instrToken.instrData->tokenSuffixes[i]) {
                        printf("ERROR: Unexpected token on line %zu: %s\n", token->line, token->str.c_str());
                        return {};
                    }
                    
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

            }
        }

        token++;
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

    std::vector<uint8_t> bytecode = assemble(fileStr);

    return 0;
}
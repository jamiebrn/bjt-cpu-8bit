#include <cstdint>
#include <fstream>
#include <stdio.h>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>

static constexpr int HEADER_SIZE = 3;
static const char* DATA_DIRECTIVE = "[data]";
static const char* PROGRAM_DIRECTIVE = "[program]";
static const char* INCLUDE_DIRECTIVE = "#include";
static const char* DEFINE_DIRECTIVE = "#define";
static const char LABEL_LOCAL_PREFIX = '.';

struct InstrData;

enum class TokenType {
    INSTR,
    REG,
    VALUE,
    STRING_LIT,
    LABEL,
    LABEL_DEF
};

struct Token {
    TokenType type;
    std::string str;

    std::string filename;
    size_t line;
    union {
        const InstrData* instrData = nullptr;
        uint8_t regValue;
    };

    bool lastInFile = false;
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
    } else if (str[0] == '\"' && str[str.size() - 1] == '\"') {
        token.type = TokenType::STRING_LIT;
        token.str = str.substr(1, str.size() - 2);
    } else if (str[str.size() - 1] == ':') {
        token.type = TokenType::LABEL_DEF;
    } else {
        token.type = TokenType::LABEL;
    }

    return token;
}

bool tokeniseFile(const std::string& filename, std::unordered_set<std::string>& includedFiles, std::vector<Token>& tokens,
    std::unordered_map<std::string, Token>& defines) {
    if (includedFiles.contains(filename)) {
        return true;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("ERROR: Could not open file %s\n", filename.c_str());
        return false;
    }

    includedFiles.insert(filename);

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    std::string text(fileSize, 0);
    file.seekg(0);
    file.read(&text[0], fileSize);
    file.close();

    std::string tokenBuffer;

    size_t line = 1;
    bool parsingComment = false;
    bool parsingInclude = false;
    bool parsingDefine = false;
    std::string defineName = "";

    for (char c : text) {
        if (parsingComment) {
            if (c == '\n') {
                parsingComment = false;
                line++;
            }
            continue;
        }

        if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9')
            && c != '.' && c != ':' && c != '[' && c != ']' && c != '#' && c != '\"' && c != '_') {
            if (!tokenBuffer.empty()) {
                if (!parsingInclude && tokenBuffer == INCLUDE_DIRECTIVE) {
                    parsingInclude = true;
                } else if (!parsingDefine && tokenBuffer == DEFINE_DIRECTIVE) {
                    parsingDefine = true;
                } else {
                    Token token = createToken(tokenBuffer, line);
                    token.filename = filename;

                    if (parsingInclude) {
                        if (token.type == TokenType::STRING_LIT) {
                            if (!tokeniseFile(token.str, includedFiles, tokens, defines)) {
                                tokens.clear();
                                return false;
                            }
                        } else {
                            printf("ERROR: Expected file name after include in file \"%s\", line %zu\n", filename.c_str(), line);
                            tokens.clear();
                            return false;
                        }

                        parsingInclude = false;
                    } else if (parsingDefine) {
                        if (defineName.empty()) {
                            if (token.type == TokenType::LABEL) {
                                if (defines.contains(token.str)) {
                                    printf("ERROR: Found repeated define \"%s\" in file \"%s\", line %zu\n", token.str.c_str(), filename.c_str(), line);
                                    tokens.clear();
                                    return false;
                                }

                                defineName = token.str;
                            } else {
                                printf("ERROR: Expected name after define in file \"%s\", line %zu\n", filename.c_str(), line);
                                tokens.clear();
                                return false;
                            }
                        } else {
                            if (token.type == TokenType::VALUE) {
                                defines[defineName] = token;
                            } else {
                                printf("ERROR: Expected value for define \"%s\" in file \"%s\", line %zu\n", defineName.c_str(), filename.c_str(), line);
                                tokens.clear();
                                return false;
                            }

                            parsingDefine = false;
                            defineName.clear();
                        }
                    } else {
                        if (token.type == TokenType::LABEL && defines.contains(token.str)) {
                            token = defines.at(token.str);
                            token.filename = filename;
                            token.line = line;
                        }

                        tokens.push_back(token);
                    }
                }

                tokenBuffer.clear();
            }

            if (c == ';') {
                parsingComment = true;
            }

            if (c == '\n') {
                line++;
            }
            continue;
        }

        tokenBuffer += c;
    }

    tokens[tokens.size() - 1].lastInFile = true;

    return true;
}

struct LabelRef {
    std::string label;
    std::string labelScope;
    std::string filename;
    size_t line;
    bool high = true;
    bool low = true;
};

bool assemblePseudoOp(std::vector<Token>& tokens, std::vector<Token>::iterator& token, std::vector<uint8_t>& bytecode,
    std::unordered_map<uint16_t, LabelRef>& labelRefs, std::string labelScope) {
    if (token->str == "cpy") {
        token++;
        if (token == tokens.end() || token->type != TokenType::REG) {
            return false;
        }
        
        uint8_t destReg = token->regValue;
        
        token++;
        if (token == tokens.end() || token->type != TokenType::REG) {
            return false;
        }

        bytecode.push_back(instructionData.at("iadd").opcode | destReg);
        bytecode.push_back(token->regValue << 4);
        bytecode.push_back(0x00);

        return true;
    } else if (token->str == "setadr") {
        token++;
        if (token == tokens.end() || token->type != TokenType::LABEL) {
            return false;
        }

        uint8_t opcode = instructionData.at("imm").opcode;
        bytecode.push_back(opcode | registerNames.at("rbnk"));
        bytecode.push_back(0x00);
        labelRefs[bytecode.size() - 1] = LabelRef{token->str, labelScope, token->filename, token->line, true, false};
        
        bytecode.push_back(opcode | registerNames.at("radr"));
        bytecode.push_back(0x00);
        labelRefs[bytecode.size() - 1] = LabelRef{token->str, labelScope, token->filename, token->line, false, true};

        return true;
    }

    return false;
}

std::vector<uint8_t> assemble(const std::string& filename) {
    std::vector<Token> tokens;
    std::unordered_set<std::string> includedFiles;
    std::unordered_map<std::string, Token> defines;

    if (!tokeniseFile(filename, includedFiles, tokens, defines)) {
        return {};
    }

    std::vector<uint8_t> bytecode(HEADER_SIZE, 0x00);
    bytecode[0] = instructionData.at("jmp").opcode;

    std::unordered_map<std::string, uint16_t> labelDefs;
    std::unordered_map<std::string, std::unordered_map<std::string, uint16_t>> labelDefsLocal;
    std::unordered_map<uint16_t, LabelRef> labelRefs;
    std::string labelScope;

    bool programMode = true;

    for (auto token = tokens.begin(); token != tokens.end();) {
        if (token->type == TokenType::LABEL_DEF) {
            if (token->str[0] == '.') {
                if (programMode) {
                    if (labelScope.empty()) {
                        printf("ERROR: Cannot define local label in global scope. File \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                        return {};
                    }

                    labelDefsLocal[labelScope][token->str.substr(0, token->str.size() - 1)] = bytecode.size();
                } else {
                    printf("ERROR: Cannot define local label in data section. File \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                    return {};
                }
            } else {
                if (labelDefs.contains(token->str)) {
                    printf("ERROR: Redefinition of label in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                    return {};
                }

                if (programMode) {
                    labelScope = token->str;
                }
    
                labelDefs[token->str.substr(0, token->str.size() - 1)] = bytecode.size();
            }

            token++;
            continue;
        }

        if (programMode) {
            if (token->str == DATA_DIRECTIVE) {
                programMode = false;
                labelScope.clear();
            } else if (token->type == TokenType::INSTR) {
                Token instrToken = *token;
                uint8_t opcode = instrToken.instrData->opcode;
                
                if (instrToken.instrData->embedDestReg) {
                    token++;
                    if (token == tokens.end() || token->type != TokenType::REG) {
                        printf("ERROR: Expected register operand in file \"%s\", on line %zu for instruction: %s\n", token->filename.c_str(), instrToken.line, instrToken.str.c_str());
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
                        printf("ERROR: Expected operand in file \"%s\", on line %zu for instruction: %s\n", token->filename.c_str(), instrToken.line, instrToken.str.c_str());
                        return {};
                    }

                    if (token->type != instrToken.instrData->tokenSuffixes[i]) {
                        printf("ERROR: Unexpected token in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
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
                        if (pushedReg) {
                            bytecode.push_back(operand << 4);
                            pushedReg = false;
                        }
                        if (parseValue(token->str, operand)) {
                            bytecode.push_back(operand);
                            operand = 0;
                        } else {
                            printf("ERROR: Failed to parse value in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                            return {};
                        }
                    } else if (token->type == TokenType::LABEL) {
                        labelRefs[bytecode.size()] = {token->str, labelScope, token->filename, token->line}; // replace with correct label addr after assembled
                        bytecode.push_back(0x00);
                        bytecode.push_back(0x00);
                    }
                }

                if (pushedReg) {
                    bytecode.push_back(operand << 4);
                }
            } else if (!assemblePseudoOp(tokens, token, bytecode, labelRefs, labelScope)) {
                printf("ERROR: Unexpected stray token in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                return {};
            }
        } else {
            if (token->str == PROGRAM_DIRECTIVE) {
                programMode = true;
                labelScope.clear();
            } else if (token->type != TokenType::VALUE) {
                printf("ERROR: Unexpected label in data section in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                return {};
            } else {
                uint8_t value;
                if (parseValue(token->str, value)) {
                    bytecode.push_back(value);
                } else {
                    printf("ERROR: Failed to parse value in file \"%s\", on line %zu: %s\n", token->filename.c_str(), token->line, token->str.c_str());
                    return {};
                }
            }
        }

        if (token->lastInFile) { // reset program mode per file
            programMode = true;
            labelScope.clear();
        }

        token++;
    }

    if (auto mainLabel = labelDefs.find("main"); mainLabel != labelDefs.end()) {
        uint16_t mainAddr = mainLabel->second;
        bytecode[1] = (mainAddr >> 8) & 0xFF;
        bytecode[2] = mainAddr & 0xFF;
    } else {
        printf("ERROR: Must define program entry point (\"main\" label)\n");
        return {};
    }

    for (auto labelRef = labelRefs.begin(); labelRef != labelRefs.end(); labelRef++) {
        uint16_t refAddr = labelRef->first;
        const std::unordered_map<std::string, uint16_t>* labelDefsPtr = &labelDefs;

        if (labelRef->second.label[0] == '.') {
            auto labelDefsIter = labelDefsLocal.find(labelRef->second.labelScope);
            if (labelDefsIter != labelDefsLocal.end()) {
                labelDefsPtr = &labelDefsIter->second;
            } else {
                printf("ERROR: Undefined label referenced in file \"%s\", on line %zu: %s\n", labelRef->second.filename.c_str(), labelRef->second.line, labelRef->second.label.c_str());
                return {};
            }
        }

        if (auto labelDef = labelDefsPtr->find(labelRef->second.label); labelDef != labelDefsPtr->end()) {
            if (labelRef->second.high) {
                bytecode[refAddr] = (labelDef->second >> 8) & 0xFF;
                refAddr++;
            }
            if (labelRef->second.low) {
                bytecode[refAddr] = labelDef->second & 0xFF;
            }
        } else {
            printf("ERROR: Undefined label referenced in file \"%s\", on line %zu: %s\n", labelRef->second.filename.c_str(), labelRef->second.line, labelRef->second.label.c_str());
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

    std::vector<uint8_t> bytecode = assemble(argv[1]);

    if (bytecode.empty()) {
        return 1;
    }

    std::string filename(argv[1]);
    std::ofstream outFile(filename.substr(0, filename.find_last_of('.')) + ".bin", std::ios::binary);
    
    for (uint8_t byte : bytecode) {
        outFile << byte;
    }
    outFile.close();

    return 0;
}
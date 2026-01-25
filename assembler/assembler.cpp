#include <cstdint>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    INSTR,
    REG,
    VALUE,
    LABEL
};

struct Token {
    TokenType type;
    std::string str;
};

struct InstrData {
    std::vector<std::vector<TokenType>> validTokenSuffixes;
};

std::unordered_map<std::string, InstrData> instructionData = {
    {"stop",    {}},
    {"ret",     {}},
    {"pop",     {{{TokenType::REG}}}},
    {"lda",     {{{TokenType::REG}}}},
    {"plda",    {{{TokenType::REG}}}},
    {"add",     {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"addc",    {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"sub",     {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"subc",    {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"imm",     {{{TokenType::REG, TokenType::VALUE}}}},
    {"nand",    {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"push",    {{{TokenType::REG}}}},
    {"sto",     {{{TokenType::REG}}}},
    {"cmp",     {{{TokenType::REG, TokenType::REG}}}},
    {"strla",   {{{TokenType::REG, TokenType::REG}}}},
    {"ldrl",    {{{TokenType::REG, TokenType::REG, TokenType::REG}}}},
    {"iadd",    {{{TokenType::REG, TokenType::REG, TokenType::VALUE}}}},
    {"isub",    {{{TokenType::REG, TokenType::REG, TokenType::VALUE}}}},
    {"jmp",     {{{TokenType::VALUE}, {TokenType::LABEL}}}},
    {"jmpz",    {{{TokenType::VALUE}, {TokenType::LABEL}}}},
    {"jmpn",    {{{TokenType::VALUE}, {TokenType::LABEL}}}},
    {"jmpc",    {{{TokenType::VALUE}, {TokenType::LABEL}}}},
    {"jmpo",    {{{TokenType::VALUE}, {TokenType::LABEL}}}},
    {"call",    {{{TokenType::VALUE}, {TokenType::LABEL}}}}
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

std::vector<Token> tokenise(const std::string& text) {
    std::vector<Token> tokens;
    std::string tokenBuffer;

    size_t parsePtr = 0;
    while (parsePtr < text.size()) {
        if (text[parsePtr] == ' ') {
            if (!tokenBuffer.empty()) {
                Token token;
                token.str = tokenBuffer;
                if (instructionData.contains(token.str)) {
                    token.type = TokenType::INSTR;
                }

            }

            tokenBuffer.clear();
        } else {
            tokenBuffer += text[parsePtr];
        }

        parsePtr++;
    }
}

std::vector<uint8_t> assemble(const std::string& text) {
    std::vector<Token> tokens = tokenise(text);
    std::vector<uint8_t> bytecode;

    for (const Token& token : tokens) {

    }
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
}
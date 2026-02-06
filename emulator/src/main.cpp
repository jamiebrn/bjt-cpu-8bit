#include <SDL2/SDL.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <fstream>
#include <vector>

#include "bjtcpu.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Must provide ROM file\n");
        return 1;
    }

    bjtcpu cpu;

    std::fstream file(argv[1], std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        printf("Could not open ROM file \"%s\"\n", argv[1]);
        return 1;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> romBin(fileSize, 0);
    file.read((char*)&romBin[0], fileSize);

    cpu.loadROM(romBin.data(), romBin.size());
    printf("Loaded ROM of %zu bytes\n", fileSize);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("bjtcpu-emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);


        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
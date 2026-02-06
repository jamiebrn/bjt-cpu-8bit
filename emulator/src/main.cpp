#include <SDL2/SDL.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <fstream>
#include <vector>
#include <chrono>

#include "bjtcpu.hpp"

void drawText(SDL_Renderer* renderer, TTF_Font* font, std::string text, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Shaded(font, text.c_str(), SDL_Color{255, 255, 255, 255}, SDL_Color{0, 0, 0, 255});
    SDL_Texture* surfaceTex = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect rect{x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, surfaceTex, NULL, &rect);
    
    SDL_DestroyTexture(surfaceTex);
    SDL_FreeSurface(surface);
}

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
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("bjtcpu-emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    TTF_Font* font = TTF_OpenFont("Rubik-Regular.ttf", 24);

    auto now = std::chrono::high_resolution_clock::now();
    auto last = now;

    float stepTime = 0;
    constexpr int CLOCK_SPEED = 100;
    constexpr float MAX_STEP_TIME = 1.0f / CLOCK_SPEED;

    bool running = true;
    while (running) {
        last = now;
        now = std::chrono::high_resolution_clock::now();
        stepTime += std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0f;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        while (stepTime >= MAX_STEP_TIME) {
            cpu.step();
            stepTime -= MAX_STEP_TIME;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        drawText(renderer, font, std::format("PC {:x}", cpu.getPCValue()), 10, 10);

        drawText(renderer, font, std::format("IR0 {:x}", cpu.getIRValue(0)), 10, 40);
        drawText(renderer, font, std::format("IR1 {:x}", cpu.getIRValue(1)), 10, 70);
        drawText(renderer, font, std::format("IR2 {:x}", cpu.getIRValue(2)), 10, 100);

        drawText(renderer, font, std::format("RA {:x}", cpu.getRegValue(REG_A)), 10, 140);
        drawText(renderer, font, std::format("RB {:x}", cpu.getRegValue(REG_B)), 10, 170);
        drawText(renderer, font, std::format("RC {:x}", cpu.getRegValue(REG_C)), 10, 200);

        drawText(renderer, font, std::format("RSP {:x}", cpu.getRegValue(REG_SP)), 10, 240);
        drawText(renderer, font, std::format("RBP {:x}", cpu.getRegValue(REG_BP)), 10, 270);

        drawText(renderer, font, std::format("RBNK {:x}", cpu.getRegValue(REG_BNK)), 10, 340);
        drawText(renderer, font, std::format("RADDR {:x}", cpu.getRegValue(REG_ADDR)), 10, 370);

        #ifdef BJTCPU_EXT_DISPLAY
        {
            SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(cpu.getDisplay().getFramebuffer(), 64, 64, 1, 64 * 3, SDL_PIXELFORMAT_RGB888);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect{300, 20, 320, 320};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
        #endif

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
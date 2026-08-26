#include <SDL2/SDL.h>
#include "cube.h"
#include <map>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const int TILE_SIZE = 40;
const int PADDING = 5;
const int OFFSET_X = 100;
const int OFFSET_Y = 50;

Cube cube;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;

std::map<Color, SDL_Color> colorMap = {
    {WHITE, {255, 255, 255, 255}},
    {YELLOW, {255, 255, 0, 255}},
    {RED, {255, 0, 0, 255}},
    {ORANGE, {255, 165, 0, 255}},
    {BLUE, {0, 0, 255, 255}},
    {GREEN, {0, 255, 0, 255}}
};

void drawFace(Face face, int x, int y) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Color c = cube.getColor(face, i, j);
            SDL_Color sdlColor = colorMap[c];
            SDL_SetRenderDrawColor(renderer, sdlColor.r, sdlColor.g, sdlColor.b, sdlColor.a);
            SDL_Rect rect = { x + j * (TILE_SIZE + PADDING), y + i * (TILE_SIZE + PADDING), TILE_SIZE, TILE_SIZE };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    int faceW = 3 * (TILE_SIZE + PADDING);

    drawFace(UP, OFFSET_X + faceW, OFFSET_Y);
    drawFace(LEFT, OFFSET_X, OFFSET_Y + faceW);
    drawFace(FRONT, OFFSET_X + faceW, OFFSET_Y + faceW);
    drawFace(RIGHT, OFFSET_X + 2 * faceW, OFFSET_Y + faceW);
    drawFace(BACK, OFFSET_X + 3 * faceW, OFFSET_Y + faceW);
    drawFace(DOWN, OFFSET_X + faceW, OFFSET_Y + 2 * faceW);

    SDL_RenderPresent(renderer);
}

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_KEYDOWN) {
            bool clockwise = !(SDL_GetModState() & KMOD_SHIFT);
            switch (event.key.keysym.sym) {
                case SDLK_u: cube.rotateFace(UP, clockwise); break;
                case SDLK_d: cube.rotateFace(DOWN, clockwise); break;
                case SDLK_l: cube.rotateFace(LEFT, clockwise); break;
                case SDLK_r: cube.rotateFace(RIGHT, clockwise); break;
                case SDLK_f: cube.rotateFace(FRONT, clockwise); break;
                case SDLK_b: cube.rotateFace(BACK, clockwise); break;
                case SDLK_s: cube.scramble(); break;
                case SDLK_ESCAPE: running = false; break;
            }
        }
    }
}

void mainLoop() {
    handleInput();
    render();
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Rubik's Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    while (running) {
        mainLoop();
        SDL_Delay(16);
    }
#endif

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

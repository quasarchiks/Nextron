#include <SDL3/SDL.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 4;
const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;
const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE;
const int FILL_PROBABILITY = 45;  // Процент заполненности стенами
const int STEPS = 5;  // Количество шагов автоматной обработки

std::vector<std::vector<int>> map;

void initializeMap() {
    map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH));
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            map[y][x] = (rand() % 100 < FILL_PROBABILITY) ? 1 : 0;
        }
    }
}

int countNeighbors(int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && ny >= 0 && nx < MAP_WIDTH && ny < MAP_HEIGHT) {
                count += map[ny][nx];
            } else {
                count++;  // Считаем край карты как стену
            }
        }
    }
    return count;
}

void cellularAutomataStep() {
    std::vector<std::vector<int>> newMap = map;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            int neighbors = countNeighbors(x, y);
            if (map[y][x] == 1) {
                newMap[y][x] = (neighbors >= 4) ? 1 : 0;
            } else {
                newMap[y][x] = (neighbors >= 5) ? 1 : 0;
            }
        }
    }
    map = newMap;
}

void generateCave() {
    initializeMap();
    for (int i = 0; i < STEPS; ++i) {
        cellularAutomataStep();
    }
}

void render(SDL_Renderer* renderer) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            SDL_FRect tile = {static_cast<float>(x * TILE_SIZE), static_cast<float>(y * TILE_SIZE), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
            if (map[y][x] == 1) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Черный
            } else {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);  // Серый
            }
            SDL_RenderFillRect(renderer, &tile);
        }
    }
    SDL_RenderPresent(renderer);
}

int main() {
    srand(static_cast<unsigned>(time(0)));
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Procedural Cave", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    generateCave();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        render(renderer);
        SDL_Delay(100);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
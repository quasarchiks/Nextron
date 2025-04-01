#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;
const int TILE_SIZE = 64;

std::vector<std::vector<int>> map; // 0 - проходимо, 1 - непроходимо

// Функция для генерации карты с использованием шума
float noise(int x, int y, float persistence = 0.5f, float frequency = 0.05f) {
    return sinf(frequency * x) * cosf(frequency * y) * persistence; // Простая синусоида для шума
}

void initializeMap() {
    map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));  // Все клетки изначально проходимы

    // Генерация основного шума для карты
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            // Применяем шум для создания проходимых и непроходимых участков
            float n = noise(x, y);
            map[y][x] = (n > 0.2f) ? 0 : 1; // Если шум выше 0.2, то проходимо, иначе непроходимо
        }
    }
}

// Функция для генерации пути через карту
void generateCavePath(int startX, int startY, int endX, int endY) {
    int currentX = startX, currentY = startY;

    // Пока не достигнем цели
    while (currentX != endX || currentY != endY) {
        map[currentY][currentX] = 0; // Сделаем текущую клетку проходимой

        // Прокладываем путь с вероятностью на основе шума
        int dx = (endX - currentX) > 0 ? 1 : -1;
        int dy = (endY - currentY) > 0 ? 1 : -1;

        if (rand() % 2 == 0) currentX += dx;
        else currentY += dy;
    }

    map[endY][endX] = 0; // Конечная точка тоже должна быть проходимой
}

// Функция для отрисовки карты на экране (SDL)
void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            SDL_FRect rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            if (map[y][x] == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Белые клетки - проходимые
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Черные клетки - непроходимые
            }
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    srand(static_cast<unsigned>(time(0)));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Cave Generator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    initializeMap();
    generateCavePath(0, 0, MAP_WIDTH - 1, MAP_HEIGHT - 1); // Прокладываем путь от верхнего левого угла к нижнему правому

    render(renderer);

    SDL_Delay(5000); // Показываем карту 5 секунд
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

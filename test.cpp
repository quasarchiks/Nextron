#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>

// Константы
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 64;
const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;

// Структура фантома
struct Phantom {
    float x, y;
    float alpha;
    std::chrono::steady_clock::time_point spawnTime;
    SDL_Texture* texture;
};

// Глобальные переменные
float x = MAP_WIDTH * TILE_SIZE / 2.0f, y = MAP_HEIGHT * TILE_SIZE / 2.0f;
float vx = 0.0f, vy = 0.0f;
const int size = 150;
const float acceleration = 2.0f;
const float friction = 0.01f;
bool running = true;

SDL_Texture* playerTextures[18];
SDL_Texture* currentTexture = nullptr;
std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
float frameTime = 0.125f;
int textureIndex = 0;

bool isDashing = false;
float dashSpeed = 10.0f;
float dashTime = 0.1f;
float dashCooldown = 1.0f;
std::chrono::steady_clock::time_point dashStartTime;
std::chrono::steady_clock::time_point lastDashTime;

std::vector<Phantom> phantoms;
float phantomLifetime = 0.5f;

std::vector<std::vector<int>> map;
SDL_Texture* groundTexture = nullptr; // Текстура для тайлов типа 0
const int FILL_PROBABILITY = 45;  // Процент заполненности стенами
const int STEPS = 5;  // Количество шагов автоматной обработки

// Функции карты
void initializeMap() {
    map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));
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

void generateCentralSquare() {
    int centerX = MAP_WIDTH / 2;
    int centerY = MAP_HEIGHT / 2;
    for (int y = centerY - 5; y < centerY + 5; ++y) {
        for (int x = centerX - 5; x < centerX + 5; ++x) {
            map[y][x] = 0;
        }
    }
}

void generateSpecialTiles() {
    std::vector<std::vector<int>> newMap = map;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (map[y][x] == 0) {
                if (x > 0 && map[y][x - 1] == 1) newMap[y][x] = 3;
                else if (x < MAP_WIDTH - 1 && map[y][x + 1] == 1) newMap[y][x] = 2;
                else if (y < MAP_HEIGHT - 1 && map[y + 1][x] == 1) newMap[y][x] = 4;
                else if (y > 0 && map[y - 1][x] == 1) newMap[y][x] = 5;
            }
        }
    }
    map = newMap;
}

void loadGroundTexture(SDL_Renderer* renderer) {
    SDL_Surface* tempSurface = IMG_Load("E:/MY PROJECTS/Nextron/assets/ground/0.png");
    if (tempSurface) {
        groundTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_DestroySurface(tempSurface);
    } else {
        std::cerr << "Ошибка загрузки текстуры ground/0.png: " << SDL_GetError() << std::endl;
    }
}

void generateCave() {
    initializeMap();
    for (int i = 0; i < STEPS; ++i) {
        cellularAutomataStep();
    }
    generateCentralSquare();
    generateSpecialTiles();
}

// Функции игрока
bool isMoving() {
    return vx != 0.0f || vy != 0.0f;
}

void loadTextures(SDL_Renderer* renderer) {
    for (int i = 0; i < 18; ++i) {
        std::string texturePath = "E:/MY PROJECTS/Nextron/assets/player/pawn/purple/pawn_" + std::to_string(i) + ".png";
        SDL_Surface* tempSurface = IMG_Load(texturePath.c_str());
        if (tempSurface) {
            playerTextures[i] = SDL_CreateTextureFromSurface(renderer, tempSurface);
            SDL_DestroySurface(tempSurface);
        } else {
            std::cerr << "Ошибка загрузки текстуры: " << SDL_GetError() << std::endl;
        }
    }
    loadGroundTexture(renderer); // Загружаем текстуру земли
}

bool checkCollision(float nextX, float nextY) {
    int tileX = static_cast<int>(nextX + size / 2) / TILE_SIZE;
    int tileY = static_cast<int>(nextY + size / 2) / TILE_SIZE;
    if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT) return true;
    return map[tileY][tileX] == 1;
}

void processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) running = false;
    }

    const bool* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W]) vy -= acceleration;
    if (keys[SDL_SCANCODE_S]) vy += acceleration;
    if (keys[SDL_SCANCODE_A]) vx -= acceleration;
    if (keys[SDL_SCANCODE_D]) vx += acceleration;

    auto currentTime = std::chrono::steady_clock::now();
    float timeSinceLastDash = std::chrono::duration<float>(currentTime - lastDashTime).count();

    if (keys[SDL_SCANCODE_LSHIFT] && timeSinceLastDash >= dashCooldown) {
        isDashing = true;
        dashStartTime = currentTime;
        lastDashTime = currentTime;
        Phantom newPhantom = {x, y, 1.0f, currentTime, currentTexture};
        phantoms.push_back(newPhantom);
    }
}

void update() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - lastTime).count();

    float nextX = x, nextY = y;
    if (isDashing) {
        vx *= dashSpeed;
        vy *= dashSpeed;
        float dashElapsed = std::chrono::duration<float>(currentTime - dashStartTime).count();
        if (dashElapsed >= dashTime) isDashing = false;
    }

    nextX += vx;
    nextY += vy;

    if (!checkCollision(nextX, y)) x = nextX;
    if (!checkCollision(x, nextY)) y = nextY;

    vx *= friction;
    vy *= friction;

    if (elapsedTime >= frameTime) {
        if (isMoving()) {
            textureIndex = (vx < 0) ? 12 + (textureIndex + 1) % 6 : 6 + (textureIndex + 1) % 6;
        } else {
            textureIndex = (textureIndex + 1) % 6;
        }
        lastTime = currentTime;
    }
    currentTexture = playerTextures[textureIndex];

    for (auto& phantom : phantoms) {
        phantom.alpha = 1.0f - (std::chrono::duration<float>(currentTime - phantom.spawnTime).count() / phantomLifetime);
    }
    phantoms.erase(std::remove_if(phantoms.begin(), phantoms.end(),
        [currentTime](const Phantom& p) { return std::chrono::duration<float>(currentTime - p.spawnTime).count() > phantomLifetime; }),
        phantoms.end());
}

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    float cameraX = x - SCREEN_WIDTH / 2.0f;
    float cameraY = y - SCREEN_HEIGHT / 2.0f;

    // Рендеринг карты
    for (int tileY = 0; tileY < MAP_HEIGHT; ++tileY) {
        for (int tileX = 0; tileX < MAP_WIDTH; ++tileX) {
            float renderX = tileX * TILE_SIZE - cameraX;
            float renderY = tileY * TILE_SIZE - cameraY;
            if (renderX + TILE_SIZE >= 0 && renderX < SCREEN_WIDTH && renderY + TILE_SIZE >= 0 && renderY < SCREEN_HEIGHT) {
                SDL_FRect tile = {renderX, renderY, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
                switch (map[tileY][tileX]) {
                    case 0: // Текстура земли
                        if (groundTexture) {
                            SDL_RenderTexture(renderer, groundTexture, NULL, &tile);
                        } else {
                            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                            SDL_RenderFillRect(renderer, &tile);
                        }
                        break;
                    case 1: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); SDL_RenderFillRect(renderer, &tile); break;
                    case 2: SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); SDL_RenderFillRect(renderer, &tile); break;
                    case 3: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); SDL_RenderFillRect(renderer, &tile); break;
                    case 4: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); SDL_RenderFillRect(renderer, &tile); break;
                    case 5: SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); SDL_RenderFillRect(renderer, &tile); break;
                }
            }
        }
    }

    // Рендеринг фантомов
    for (const auto& phantom : phantoms) {
        SDL_FRect destRect = {phantom.x - cameraX, phantom.y - cameraY, static_cast<float>(size), static_cast<float>(size)};
        SDL_SetTextureAlphaMod(phantom.texture, static_cast<Uint8>(phantom.alpha * 255));
        SDL_RenderTexture(renderer, phantom.texture, NULL, &destRect);
    }

    // Рендеринг игрока
    SDL_SetTextureAlphaMod(currentTexture, 255);
    SDL_FRect destRect = {x - cameraX, y - cameraY, static_cast<float>(size), static_cast<float>(size)};
    SDL_RenderTexture(renderer, currentTexture, NULL, &destRect);

    SDL_RenderPresent(renderer);
}

int main() {
    srand(static_cast<unsigned>(time(0)));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Cave Explorer", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    generateCave();
    loadTextures(renderer);

    while (running) {
        processInput();
        update();
        render(renderer);
        SDL_Delay(16);
    }

    for (int i = 0; i < 18; ++i) SDL_DestroyTexture(playerTextures[i]);
    if (groundTexture) SDL_DestroyTexture(groundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
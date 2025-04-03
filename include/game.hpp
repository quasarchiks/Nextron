#pragma once
#include "main.hpp"
#include "structs.hpp"
#include "dungeon_generator.hpp"
#include <algorithm>

class Game {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    Player player;
    Phantom phantom;         // Заменяем вектор на один фантом
    bool phantomActive = false; // Флаг активности фантома
    std::vector<std::vector<int>> map;
    SDL_Texture* groundTexture = nullptr;
    float phantomLifetime = 0.3f; // Уже есть, оставляем как есть
    DungeonGenerator dungeon;

    int screenWidth = 1200;
    int screenHeight = 900;

    float cameraWindowWidth;
    float cameraWindowHeight;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float targetCameraX = 0.0f;
    float targetCameraY = 0.0f;
    float cameraLerpSpeed = 3.0f;

    int minimapWidth;
    int minimapHeight;
    int minimapX;
    int minimapY;

    void loadTextures();
    void loadGroundTexture();
    bool checkCollision(float nextX, float nextY);
    void processInput();
    void update();
    void render();
    void renderMinimap(float playerX, float playerY);
    void updateCamera();

public:
    Game() : dungeon(map) {}
    bool initialize();
    void run();
    void cleanup();
};
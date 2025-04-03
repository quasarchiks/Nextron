#pragma once
#include "main.hpp"

struct Phantom {
    float x, y;
    float alpha;
    std::chrono::steady_clock::time_point spawnTime;
    SDL_Texture* texture;
};

struct Player {
    float x, y;
    float vx, vy;
    float ACCELERATION = 1.0f;
    float FRICTION = 0.9f;
    std::chrono::steady_clock::time_point lastTime;
    float frameTime = 0.1f;
    int textureIndex = 0;
    SDL_Texture* textures[48]; // Изначально было 48 текстур
    SDL_Texture* currentTexture;
    static const int SIZE = 150;
    bool isAttacking = false; // Для атаки
    std::chrono::steady_clock::time_point attackStartTime;
    float attackDuration = 0.4f;
};


//   _   _           _                   
//   | \ | |         | |                  
//   |  \| | _____  _| |_ _ __ ___  _ __  
//   | . ` |/ _ \ \/ / __| '__/ _ \| '_ \ 
//   | |\  |  __/>  <| |_| | | (_) | | | |
//   \_| \_/\___/_/\_\\__|_|  \___/|_| |_|
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <Windows.h>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>


struct Phantom {
    float x, y;
    float alpha;
    std::chrono::steady_clock::time_point spawnTime;
    SDL_Texture* texture;
};



//                        _       
//                       | |      
//     ___ ___  _ __  ___| |_ ___ 
//    / __/ _ \| '_ \/ __| __/ __|
//   | (_| (_) | | | \__ \ |_\__ \
//    \___\___/|_| |_|___/\__|___/
const int WIDTH = 800, HEIGHT = 600;

float x = WIDTH / 2.0f, y = HEIGHT / 2.0f;
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
float phantomLifetime = 0.5f; // Время жизни фантома в секундах

//    __                  
//   / _|                 
//   | |_ _   _ _ __   ___ 
//   |  _| | | | '_ \ / __|
//   | | | |_| | | | | (__ 
//   |_|  \__,_|_| |_|\___|
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
}

void processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        } 
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
    
        // Создаем фантом
        Phantom newPhantom = {x, y, 1.0f, currentTime, currentTexture};
        phantoms.push_back(newPhantom);
    }
    
}

void update() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - lastTime).count();

    if (isDashing) {
        vx *= dashSpeed;
        vy *= dashSpeed;

        float dashElapsed = std::chrono::duration<float>(currentTime - dashStartTime).count();
        if (dashElapsed >= dashTime) {
            isDashing = false;
        }
    }

    x += vx;
    y += vy;

    if (x + size/3 < 0 || x + size/3*2 > WIDTH) {
        vx = -vx;
        x = x < 0 ? 0 - size/3 : WIDTH - size/3*2;
    }
    if (y + size/3 < 0 || y + size/3*2 > HEIGHT) {
        vy = -vy;
        y = y < 0 ? 0 - size/3 : HEIGHT - size/3*2;
    }

    vx *= friction;
    vy *= friction;

    if (elapsedTime >= frameTime) {
        if (isMoving()) {
            if (vx < 0) {
                textureIndex = 12 + (textureIndex + 1) % 6;
            } else {
                textureIndex = 6 + (textureIndex + 1) % 6;
            }
        } else {
            textureIndex = (textureIndex + 1) % 6;
        }
        lastTime = currentTime;
    }

    currentTexture = playerTextures[textureIndex];

    // Обновляем фантомов (уменьшаем прозрачность)
    for (auto& phantom : phantoms) {
        float lifeTime = std::chrono::duration<float>(currentTime - phantom.spawnTime).count();
        phantom.alpha = 1.0f - (lifeTime / phantomLifetime);
    }

    // Удаляем старые фантомы
    phantoms.erase(std::remove_if(phantoms.begin(), phantoms.end(),
        [currentTime](const Phantom& p) { return std::chrono::duration<float>(currentTime - p.spawnTime).count() > phantomLifetime; }),
        phantoms.end());
}



void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Рисуем фантомов
    for (const auto& phantom : phantoms) {
        SDL_FRect destRect = { phantom.x, phantom.y, static_cast<float>(size), static_cast<float>(size) };

        // Устанавливаем прозрачность фантома
        SDL_SetTextureAlphaMod(phantom.texture, static_cast<Uint8>(phantom.alpha * 255));
        SDL_RenderTexture(renderer, phantom.texture, NULL, &destRect);
    }

    // **Сбрасываем прозрачность перед отрисовкой игрока**
    SDL_SetTextureAlphaMod(currentTexture, 255);

    // Рисуем игрока
    SDL_FRect destRect = { x, y, static_cast<float>(size), static_cast<float>(size) };
    SDL_RenderTexture(renderer, currentTexture, NULL, &destRect);

    SDL_RenderPresent(renderer);
}




//                    _       
//                   (_)      
//   _ __ ___   __ _ _ _ __  
//   | '_ ` _ \ / _` | | '_ \ 
//   | | | | | | (_| | | | | |
//   |_| |_| |_|\__,_|_|_| |_|
int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Player Texture", WIDTH, HEIGHT, 0);
    if (!window) {
        std::cerr << "Ошибка создания окна: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "Ошибка создания рендера: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    loadTextures(renderer);

    if (!playerTextures[0]) {
        std::cerr << "Ошибка создания текстуры!" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    while (running) {
        processInput();
        update();
        render(renderer);
        SDL_Delay(16);
    }

    for (int i = 0; i < 12; ++i) {
        SDL_DestroyTexture(playerTextures[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
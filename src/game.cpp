#include "game.hpp"

bool Game::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Cave Explorer", 0, 0, 
                             SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);
    renderer = SDL_CreateRenderer(window, nullptr);
    
    if (!window || !renderer) {
        std::cerr << "Ошибка создания окна или рендера: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GetWindowSize(window, &screenWidth, &screenHeight);

    cameraWindowWidth = screenWidth * 0.2f;
    cameraWindowHeight = screenHeight * 0.2f;

    minimapWidth = MAP_WIDTH * MINIMAP_TILE_SIZE;
    minimapHeight = MAP_HEIGHT * MINIMAP_TILE_SIZE;
    minimapX = screenWidth - minimapWidth - 10;
    minimapY = 10;

    bool switchForStart = false;
    if (!dungeon.generate(player.x, player.y, switchForStart)) {
        std::cout << "Dungeon generation failed!\n";
        return false;
    }
    //dungeon.print();
    
    loadTextures();

    cameraX = player.x - screenWidth / 2.0f;
    cameraY = player.y - screenHeight / 2.0f;
    targetCameraX = cameraX;
    targetCameraY = cameraY;

    return true;
}

void Game::loadTextures() {
    for (int i = 0; i <= 19; ++i) {
        std::string path = "assets/player/" + std::to_string(i) + ".png";
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface) {
            player.textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
        } else {
            std::cerr << "Ошибка загрузки текстуры: " << SDL_GetError() << std::endl;
        }
    }
    player.currentTexture = player.textures[0];
    loadGroundTexture();
}

void Game::loadGroundTexture() {
    SDL_Surface* surface = IMG_Load("assets/ground/0.png");
    if (surface) {
        groundTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    } else {
        std::cerr << "Ошибка загрузки ground/0.png: " << SDL_GetError() << std::endl;
    }
}

bool Game::checkCollision(float nextX, float nextY) {
    int tileX = static_cast<int>(nextX + player.SIZE / 2) / TILE_SIZE;
    int tileY = static_cast<int>(nextY + player.SIZE / 2) / TILE_SIZE;
    if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT) return true;
    return map[tileY][tileX] == WALL;
}

void Game::processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) running = false;
        // Добавляем атаку ПКМ (оставляем, так как это было добавлено позже)
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT) {
            if (!player.isAttacking) {
                player.isAttacking = true;
                player.attackStartTime = std::chrono::steady_clock::now();
                player.textureIndex = 16; // Начало анимации атаки
            }
        }
    }

    const bool* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W]) player.vy -= player.ACCELERATION;
    if (keys[SDL_SCANCODE_S]) player.vy += player.ACCELERATION;
    if (keys[SDL_SCANCODE_A]) player.vx -= player.ACCELERATION;
    if (keys[SDL_SCANCODE_D]) player.vx += player.ACCELERATION;
}

void Game::updateCamera() {
    float windowLeft = cameraX + (screenWidth - cameraWindowWidth) / 2.0f;
    float windowRight = windowLeft + cameraWindowWidth;
    float windowTop = cameraY + (screenHeight - cameraWindowHeight) / 2.0f;
    float windowBottom = windowTop + cameraWindowHeight;

    if (player.x < windowLeft) {
        targetCameraX -= (windowLeft - player.x);
    } else if (player.x > windowRight) {
        targetCameraX += (player.x - windowRight);
    }

    if (player.y < windowTop) {
        targetCameraY -= (windowTop - player.y);
    } else if (player.y > windowBottom) {
        targetCameraY += (player.y - windowBottom);
    }

    float deltaTime = 1.0f / 60.0f;
    float distanceX = targetCameraX - cameraX;
    float distanceY = targetCameraY - cameraY;
    float lerpFactor = cameraLerpSpeed * deltaTime;
    lerpFactor = std::min(1.0f, lerpFactor * (1.0f + std::sqrt(distanceX * distanceX + distanceY * distanceY) / 100.0f));

    cameraX += distanceX * lerpFactor;
    cameraY += distanceY * lerpFactor;
}

void Game::update() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - player.lastTime).count();

    float nextX = player.x, nextY = player.y;

    nextX += player.vx;
    nextY += player.vy;

    if (!checkCollision(nextX, player.y)) player.x = nextX;
    if (!checkCollision(player.x, nextY)) player.y = nextY;

    player.vx *= player.FRICTION;
    player.vy *= player.FRICTION;

    updateCamera();

    bool isMoving = player.vx != 0.0f || player.vy != 0.0f;

    if (player.isAttacking) {
        float attackElapsed = std::chrono::duration<float>(currentTime - player.attackStartTime).count();
        if (attackElapsed >= player.attackDuration) {
            player.isAttacking = false;
            player.textureIndex = 0; // Возвращаемся к Idle
        } else if (elapsedTime >= player.frameTime) {
            player.textureIndex = 16 + static_cast<int>(attackElapsed / player.frameTime) % 4;
            player.lastTime = currentTime;
        }
    } else {
        if (elapsedTime >= player.frameTime) {
            if (isMoving) {
                player.textureIndex = (player.vx < 0) ? 12 + (player.textureIndex + 1) % 6 : 
                                                       6 + (player.textureIndex + 1) % 6;
            } else {
                player.textureIndex = (player.textureIndex + 1) % 6;
            }
            player.lastTime = currentTime;
        }
    }
    player.currentTexture = player.textures[player.textureIndex];
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int tileY = 0; tileY < MAP_HEIGHT; ++tileY) {
        for (int tileX = 0; tileX < MAP_WIDTH; ++tileX) {
            float renderX = tileX * TILE_SIZE - cameraX;
            float renderY = tileY * TILE_SIZE - cameraY;
            if (renderX + TILE_SIZE >= 0 && renderX < screenWidth && 
                renderY + TILE_SIZE >= 0 && renderY < screenHeight) {
                SDL_FRect tile = {renderX, renderY, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
                switch (map[tileY][tileX]) {
                    case FLOOR:
                        if (groundTexture) {
                            SDL_RenderTexture(renderer, groundTexture, NULL, &tile);
                        } else {
                            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                            SDL_RenderFillRect(renderer, &tile);
                        }
                        break;
                    case WALL: 
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
                        SDL_RenderFillRect(renderer, &tile); 
                        break;
                }
            }
        }
    }

    SDL_SetTextureAlphaMod(player.currentTexture, 255);
    SDL_FRect destRect = {player.x - cameraX, player.y - cameraY, 
                          static_cast<float>(player.SIZE), static_cast<float>(player.SIZE)};
    SDL_RenderTexture(renderer, player.currentTexture, NULL, &destRect);

    renderMinimap(player.x, player.y);
    SDL_RenderPresent(renderer);
}

void Game::renderMinimap(float playerX, float playerY) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 128);
    SDL_FRect minimapBackground = {static_cast<float>(minimapX - 2), static_cast<float>(minimapY - 2),
                                  static_cast<float>(minimapWidth + 4), static_cast<float>(minimapHeight + 4)};
    SDL_RenderFillRect(renderer, &minimapBackground);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            SDL_FRect tile = {static_cast<float>(minimapX + x * MINIMAP_TILE_SIZE),
                            static_cast<float>(minimapY + y * MINIMAP_TILE_SIZE),
                            static_cast<float>(MINIMAP_TILE_SIZE), static_cast<float>(MINIMAP_TILE_SIZE)};
            SDL_SetRenderDrawColor(renderer, map[y][x] == FLOOR ? 128 : 0, 
                                  map[y][x] == FLOOR ? 128 : 0, 
                                  map[y][x] == FLOOR ? 128 : 0, 255);
            SDL_RenderFillRect(renderer, &tile);
        }
    }

    float playerMapX = minimapX + (playerX / TILE_SIZE) * MINIMAP_TILE_SIZE;
    float playerMapY = minimapY + (playerY / TILE_SIZE) * MINIMAP_TILE_SIZE;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_FRect playerRect = {playerMapX - 1, playerMapY - 1, 4, 4};
    SDL_RenderFillRect(renderer, &playerRect);
}

void Game::run() {
    while (running) {
        processInput();
        update();
        render();
        SDL_Delay(16);
    }
}

void Game::cleanup() {
    for (int i = 0; i < 20; ++i) {
        if (player.textures[i]) SDL_DestroyTexture(player.textures[i]);
    }
    if (groundTexture) SDL_DestroyTexture(groundTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
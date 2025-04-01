#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <random>
#include <cmath>

struct Phantom {
    float x, y;
    float alpha;
    std::chrono::steady_clock::time_point spawnTime;
    SDL_Texture* texture;
};

enum TileType {
    WALL = 1,
    FLOOR = 0
};

float x, y;

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 900;
const int TILE_SIZE = 64;
const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;

float vx = 0.0f, vy = 0.0f;
const int size = 150;
const float acceleration = 4.0f;
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
float dashCooldown = 0.3f;
std::chrono::steady_clock::time_point dashStartTime;
std::chrono::steady_clock::time_point lastDashTime;

std::vector<Phantom> phantoms;
float phantomLifetime = 0.5f;

std::vector<std::vector<int>> map;
SDL_Texture* groundTexture = nullptr;

const int MINIMAP_TILE_SIZE = 2;
const int MINIMAP_WIDTH = MAP_WIDTH * MINIMAP_TILE_SIZE;
const int MINIMAP_HEIGHT = MAP_HEIGHT * MINIMAP_TILE_SIZE;
const int MINIMAP_X = SCREEN_WIDTH - MINIMAP_WIDTH - 10;
const int MINIMAP_Y = 10;

bool switchForStart = false;

class DungeonGenerator {
    private:
        std::random_device rd;
        std::mt19937 gen;
    
        struct CircleRoom {
            int x, y;  // Center coordinates
            int radius;
        };
    
        void initializeMap() {
            map.clear(); // Ensure clean slate
            map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, WALL));
            std::cout << "Map initialized: " << MAP_HEIGHT << "x" << MAP_WIDTH << "\n";
        }
    
        bool isValidPosition(int x, int y) const {
            return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
        }
    
        bool isValidCirclePlacement(const CircleRoom& room, const std::vector<CircleRoom>& existingRooms) {
            if (!isValidPosition(room.x - room.radius - 1, room.y - room.radius - 1) ||
                !isValidPosition(room.x + room.radius + 1, room.y + room.radius + 1)) {
                return false;
            }
    
            for (const auto& existing : existingRooms) {
                float distance = std::sqrt((room.x - existing.x) * (room.x - existing.x) + 
                                         (room.y - existing.y) * (room.y - existing.y));
                if (distance < (room.radius + existing.radius + 5)) {
                    return false;
                }
            }
            return true;
        }
    
        void createCircleRoom(const CircleRoom& room) {
            std::cout << "Creating room at (" << room.x << "," << room.y << ") with radius " << room.radius << "\n";
            for (int y = std::max(0, room.y - room.radius); 
                 y <= std::min(MAP_HEIGHT - 1, room.y + room.radius); y++) {
                for (int x = std::max(0, room.x - room.radius); 
                     x <= std::min(MAP_WIDTH - 1, room.x + room.radius); x++) {
                    float distance = std::sqrt((x - room.x) * (x - room.x) + 
                                            (y - room.y) * (y - room.y));
                    if (distance <= room.radius) {
                        map[y][x] = FLOOR;
                    }
                }
            }
        }
    
        void createWindingCorridor(int x1, int y1, int x2, int y2) {
            std::cout << "Connecting (" << x1 << "," << y1 << ") to (" << x2 << "," << y2 << ")\n";
            std::uniform_int_distribution<> widthDist(1, 3);
            std::uniform_int_distribution<> offsetDist(-1, 1);
            
            int x = std::clamp(x1, 0, MAP_WIDTH - 1);
            int y = std::clamp(y1, 0, MAP_HEIGHT - 1);
            
            while (x != x2 || y != y2) {
                int width = widthDist(gen);
                for (int wy = -width; wy <= width; wy++) {
                    for (int wx = -width; wx <= width; wx++) {
                        int newX = std::clamp(x + wx, 0, MAP_WIDTH - 1);
                        int newY = std::clamp(y + wy, 0, MAP_HEIGHT - 1);
                        map[newY][newX] = FLOOR;
                    }
                }
    
                if (x < x2) x = std::min(x + 1 + offsetDist(gen), MAP_WIDTH - 1);
                else if (x > x2) x = std::max(x - 1 + offsetDist(gen), 0);
                
                if (y < y2) y = std::min(y + 1 + offsetDist(gen), MAP_HEIGHT - 1);
                else if (y > y2) y = std::max(y - 1 + offsetDist(gen), 0);
            }
        }
    
        void smoothMap(int iterations) {
            std::cout << "Smoothing map with " << iterations << " iterations\n";
            for (int i = 0; i < iterations; i++) {
                std::vector<std::vector<int>> newMap = map;
                for (int y = 1; y < MAP_HEIGHT - 1; y++) {
                    for (int x = 1; x < MAP_WIDTH - 1; x++) {
                        int neighborWalls = 0;
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (map[y + dy][x + dx] == WALL) neighborWalls++;
                            }
                        }
                        if (neighborWalls > 4) newMap[y][x] = WALL;
                        else if (neighborWalls < 4) newMap[y][x] = FLOOR;
                    }
                }
                map = newMap;
            }
        }
    
    public:
        DungeonGenerator() : gen(rd()) {
            initializeMap();
        }
    
        bool generate() {
            const int DIAMETER = 16;
            const int RADIUS = DIAMETER / 2;
            std::vector<CircleRoom> rooms;
            std::uniform_int_distribution<> numRoomsDist(5, 10);
            std::uniform_int_distribution<> yDist(8, MAP_HEIGHT - 8);
    
            int numRooms = numRoomsDist(gen);
            std::cout << "Generating " << numRooms << " rooms\n";
            int xStep = (MAP_WIDTH - 20) / std::max(1, numRooms - 1);
    
            // Place rooms with guaranteed minimum
            for (int i = 0; i < numRooms; i++) {
                CircleRoom room;
                int attempts = 0;
                const int maxAttempts = 50;
    
                do {
                    room.x = 10 + (i * xStep) + (gen() % 10 - 5);
                    room.y = yDist(gen);
                    if (!switchForStart) {
                        x = room.x * TILE_SIZE;
                        y = room.y * TILE_SIZE;
                        switchForStart = true;
                        std::cout<<"START COORDS "<<room.x<<" | "<<room.y<<std::endl;
                    }
                    room.radius = RADIUS;
                    attempts++;
                } while (!isValidCirclePlacement(room, rooms) && attempts < maxAttempts);
    
                if (attempts >= maxAttempts) {
                    room.x = 10 + (i * xStep);
                    room.y = MAP_HEIGHT / 2;
                }
    
                if (isValidCirclePlacement(room, rooms)) {
                    createCircleRoom(room);
                    rooms.push_back(room);
                }
            }
    
            if (rooms.size() < 5) {
                std::cerr << "Only placed " << rooms.size() << " rooms, minimum 5 required\n";
                // Force minimum 5 rooms
                while (rooms.size() < 5) {
                    CircleRoom room;
                    room.x = 10 + (rooms.size() * xStep);
                    room.y = MAP_HEIGHT / 2;
                    if (isValidCirclePlacement(room, rooms)) {
                        createCircleRoom(room);
                        rooms.push_back(room);
                    } else {
                        std::cerr << "Failed to place minimum rooms\n";
                        return false;
                    }
                }
            }
    
            std::cout << "Placed " << rooms.size() << " rooms\n";
    
            // Connect rooms sequentially
            for (size_t i = 1; i < rooms.size(); i++) {
                createWindingCorridor(rooms[i-1].x, rooms[i-1].y, rooms[i].x, rooms[i].y);
            }
    
            smoothMap(2);
            return true;
        }
    
        void print() const {
            std::cout << "Printing map\n";
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    std::cout << (map[y][x] == WALL ? "#" : ".");
                }
                std::cout << "\n";
            }
        }
    };

DungeonGenerator dungeon;

void initializeMap() {
    map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));
    
}

void generateCave() {
    if (dungeon.generate()) {
        dungeon.print();
    } else {
        std::cout << "Dungeon generation failed!\n";
    }
}

bool isMoving() {
    return vx != 0.0f || vy != 0.0f;
}

void renderMinimap(SDL_Renderer* renderer, float playerX, float playerY) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 128);
    SDL_FRect minimapBackground = {
        static_cast<float>(MINIMAP_X - 2),
        static_cast<float>(MINIMAP_Y - 2),
        static_cast<float>(MINIMAP_WIDTH + 4),
        static_cast<float>(MINIMAP_HEIGHT + 4)
    };
    SDL_RenderFillRect(renderer, &minimapBackground);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            SDL_FRect tile = {
                static_cast<float>(MINIMAP_X + x * MINIMAP_TILE_SIZE),
                static_cast<float>(MINIMAP_Y + y * MINIMAP_TILE_SIZE),
                static_cast<float>(MINIMAP_TILE_SIZE),
                static_cast<float>(MINIMAP_TILE_SIZE)
            };
            
            switch (map[y][x]) {
                case 0: SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); break;
                case 1: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); break;
                case 2: SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); break;
                case 3: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;
                case 4: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;
                case 5: SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); break;
            }
            SDL_RenderFillRect(renderer, &tile);
        }
    }

    float playerMapX = MINIMAP_X + (playerX / TILE_SIZE) * MINIMAP_TILE_SIZE;
    float playerMapY = MINIMAP_Y + (playerY / TILE_SIZE) * MINIMAP_TILE_SIZE;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_FRect playerRect = {
        playerMapX - 1,
        playerMapY - 1,
        4, 4
    };
    SDL_RenderFillRect(renderer, &playerRect);
}

void loadGroundTexture(SDL_Renderer* renderer) {
    SDL_Surface* tempSurface = IMG_Load("assets/ground/0.png");
    if (tempSurface) {
        groundTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_DestroySurface(tempSurface);
    } else {
        std::cerr << "Ошибка загрузки текстуры ground/0.png: " << SDL_GetError() << std::endl;
    }
}

void loadTextures(SDL_Renderer* renderer) {
    for (int i = 0; i < 18; ++i) {
        std::string texturePath = "assets/player/pawn/purple/pawn_" + std::to_string(i) + ".png";
        SDL_Surface* tempSurface = IMG_Load(texturePath.c_str());
        if (tempSurface) {
            playerTextures[i] = SDL_CreateTextureFromSurface(renderer, tempSurface);
            SDL_DestroySurface(tempSurface);
        } else {
            std::cerr << "Ошибка загрузки текстуры: " << SDL_GetError() << std::endl;
        }
    }
    loadGroundTexture(renderer);
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

    for (int tileY = 0; tileY < MAP_HEIGHT; ++tileY) {
        for (int tileX = 0; tileX < MAP_WIDTH; ++tileX) {
            float renderX = tileX * TILE_SIZE - cameraX;
            float renderY = tileY * TILE_SIZE - cameraY;
            if (renderX + TILE_SIZE >= 0 && renderX < SCREEN_WIDTH && renderY + TILE_SIZE >= 0 && renderY < SCREEN_HEIGHT) {
                SDL_FRect tile = {renderX, renderY, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
                switch (map[tileY][tileX]) {
                    case 0:
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

    for (const auto& phantom : phantoms) {
        SDL_FRect destRect = {phantom.x - cameraX, phantom.y - cameraY, static_cast<float>(size), static_cast<float>(size)};
        SDL_SetTextureAlphaMod(phantom.texture, static_cast<Uint8>(phantom.alpha * 255));
        SDL_RenderTexture(renderer, phantom.texture, NULL, &destRect);
    }

    SDL_SetTextureAlphaMod(currentTexture, 255);
    SDL_FRect destRect = {x - cameraX, y - cameraY, static_cast<float>(size), static_cast<float>(size)};
    SDL_RenderTexture(renderer, currentTexture, NULL, &destRect);

    renderMinimap(renderer, x, y);

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
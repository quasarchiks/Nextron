#include "dungeon_generator.hpp"

bool DungeonGenerator::isValidPosition(int x, int y) const {
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

bool DungeonGenerator::isValidCirclePlacement(const CircleRoom& room, const std::vector<CircleRoom>& existingRooms) {
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

void DungeonGenerator::createCircleRoom(const CircleRoom& room) {
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

void DungeonGenerator::createWindingCorridor(int x1, int y1, int x2, int y2) {
    std::uniform_int_distribution<> widthDist(1, 3);
    std::uniform_int_distribution<> offsetDist(-1, 1);
    
    int x = clamp(x1, 0, MAP_WIDTH - 1);
    int y = clamp(y1, 0, MAP_HEIGHT - 1);
    
    while (x != x2 || y != y2) {
        int width = widthDist(gen);
        for (int wy = -width; wy <= width; wy++) {
            for (int wx = -width; wx <= width; wx++) {
                int newX = clamp(x + wx, 0, MAP_WIDTH - 1);
                int newY = clamp(y + wy, 0, MAP_HEIGHT - 1);
                map[newY][newX] = FLOOR;
            }
        }

        if (x < x2) x = std::min(x + 1 + offsetDist(gen), MAP_WIDTH - 1);
        else if (x > x2) x = std::max(x - 1 + offsetDist(gen), 0);
        
        if (y < y2) y = std::min(y + 1 + offsetDist(gen), MAP_HEIGHT - 1);
        else if (y > y2) y = std::max(y - 1 + offsetDist(gen), 0);
    }
}

void DungeonGenerator::smoothMap(int iterations) {
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

bool DungeonGenerator::generate(float& startX, float& startY, bool& switchForStart) {
    constexpr int DIAMETER = 16;
    constexpr int RADIUS = DIAMETER / 2;
    std::vector<CircleRoom> rooms;
    std::uniform_int_distribution<> numRoomsDist(5, 10);
    std::uniform_int_distribution<> yDist(8, MAP_HEIGHT - 8);

    int numRooms = numRoomsDist(gen);
    int xStep = (MAP_WIDTH - 20) / std::max(1, numRooms - 1);

    for (int i = 0; i < numRooms; i++) {
        CircleRoom room;
        int attempts = 0;
        const int maxAttempts = 50;

        do {
            room.x = 10 + (i * xStep) + (gen() % 10 - 5);
            room.y = yDist(gen);
            if (!switchForStart) {
                startX = room.x * TILE_SIZE;
                startY = room.y * TILE_SIZE;
                switchForStart = true;
                std::cout<<room.x<<room.y<<std::endl;
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

    while (rooms.size() < 5) {
        CircleRoom room;
        room.x = 10 + (rooms.size() * xStep);
        room.y = MAP_HEIGHT / 2;
        if (isValidCirclePlacement(room, rooms)) {
            createCircleRoom(room);
            rooms.push_back(room);
        } else {
            return false;
        }
    }

    for (size_t i = 1; i < rooms.size(); i++) {
        createWindingCorridor(rooms[i-1].x, rooms[i-1].y, rooms[i].x, rooms[i].y);
    }

    smoothMap(2);
    return true;
}

void DungeonGenerator::print() const {
    for (const auto& row : map) {
        for (int tile : row) {
            std::cout << (tile == WALL ? "#" : ".");
        }
        std::cout << "\n";
    }
}
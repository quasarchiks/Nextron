#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>
#include <cmath>

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;

enum TileType {
    WALL = 0,
    FLOOR = 1
};

class DungeonGenerator {
private:
    std::vector<std::vector<int>> map;
    std::random_device rd;
    std::mt19937 gen;

    struct CircleRoom {
        int x, y;  // Center coordinates
        int radius;
    };

    void initializeMap() {
        map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, WALL));
    }

    bool isValidCirclePlacement(const CircleRoom& room, const std::vector<CircleRoom>& existingRooms) {
        if (room.x - room.radius - 1 < 0 || room.x + room.radius + 1 >= MAP_WIDTH ||
            room.y - room.radius - 1 < 0 || room.y + room.radius + 1 >= MAP_HEIGHT) {
            return false;
        }

        for (const auto& existing : existingRooms) {
            float distance = std::sqrt((room.x - existing.x) * (room.x - existing.x) + 
                                     (room.y - existing.y) * (room.y - existing.y));
            if (distance < (room.radius + existing.radius + 5)) { // Minimum separation
                return false;
            }
        }
        return true;
    }

    void createCircleRoom(const CircleRoom& room) {
        for (int y = room.y - room.radius; y <= room.y + room.radius; y++) {
            for (int x = room.x - room.radius; x <= room.x + room.radius; x++) {
                float distance = std::sqrt((x - room.x) * (x - room.x) + 
                                        (y - room.y) * (y - room.y));
                if (distance <= room.radius) {
                    map[y][x] = FLOOR;
                }
            }
        }
    }

    void createWindingCorridor(int x1, int y1, int x2, int y2) {
        std::uniform_int_distribution<> widthDist(1, 3);
        std::uniform_int_distribution<> offsetDist(-1, 1);
        
        int x = x1;
        int y = y1;
        
        while (x != x2 || y != y2) {
            int width = widthDist(gen);
            for (int wy = -width; wy <= width; wy++) {
                for (int wx = -width; wx <= width; wx++) {
                    int newX = x + wx;
                    int newY = y + wy;
                    if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
                        map[newY][newX] = FLOOR;
                    }
                }
            }

            if (x < x2) x += 1 + offsetDist(gen);
            else if (x > x2) x -= 1 + offsetDist(gen);
            
            if (y < y2) y += 1 + offsetDist(gen);
            else if (y > y2) y -= 1 + offsetDist(gen);

            x = std::clamp(x, 0, MAP_WIDTH - 1);
            y = std::clamp(y, 0, MAP_HEIGHT - 1);
        }
    }

    void smoothMap(int iterations) {
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

    void generate() {
        const int DIAMETER = 16;
        const int RADIUS = DIAMETER / 2;
        std::vector<CircleRoom> rooms;
        std::uniform_int_distribution<> numRoomsDist(5, 10); // Random 5-10 rooms
        std::uniform_int_distribution<> yDist(8, MAP_HEIGHT - 8);

        int numRooms = numRoomsDist(gen);
        int xStep = (MAP_WIDTH - 20) / (numRooms - 1); // Evenly space rooms across width

        // Place rooms in linear progression
        for (int i = 0; i < numRooms; i++) {
            CircleRoom room;
            int attempts = 0;
            const int maxAttempts = 50;

            do {
                // Linear progression with some randomness
                room.x = 10 + (i * xStep) + (gen() % 10 - 5); // Base x + small random offset
                room.y = yDist(gen);
                room.radius = RADIUS;
                attempts++;
            } while (!isValidCirclePlacement(room, rooms) && attempts < maxAttempts);

            if (attempts < maxAttempts) {
                createCircleRoom(room);
                rooms.push_back(room);
            } else {
                // If placement fails, try a simpler position
                room.x = 10 + (i * xStep);
                room.y = MAP_HEIGHT / 2;
                if (isValidCirclePlacement(room, rooms)) {
                    createCircleRoom(room);
                    rooms.push_back(room);
                }
            }
        }

        // Connect rooms sequentially
        for (size_t i = 1; i < rooms.size(); i++) {
            createWindingCorridor(rooms[i-1].x, rooms[i-1].y, rooms[i].x, rooms[i].y);
        }

        // Apply smoothing
        smoothMap(2);
    }

    void print() const {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                std::cout << (map[y][x] == WALL ? "#" : ".");
            }
            std::cout << std::endl;
        }
    }

    const std::vector<std::vector<int>>& getMap() const {
        return map;
    }

    std::pair<int, int> getStartPosition() const { return {10, MAP_HEIGHT / 2}; }
    std::pair<int, int> getEndPosition() const { return {MAP_WIDTH - 10, MAP_HEIGHT / 2}; }
};

int main() {
    DungeonGenerator dungeon;
    dungeon.generate();
    dungeon.print();
    
    auto start = dungeon.getStartPosition();
    auto end = dungeon.getEndPosition();
    std::cout << "\nStart: (" << start.first << "," << start.second << ")\n";
    std::cout << "End: (" << end.first << "," << end.second << ")\n";
    return 0;
}
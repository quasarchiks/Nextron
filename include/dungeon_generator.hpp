#pragma once
#include "main.hpp"
#include <algorithm> // Для std::max и std::min

class DungeonGenerator {
private:
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::vector<std::vector<int>>& map;

    struct CircleRoom {
        int x, y;
        int radius;
    };

    // Вспомогательная функция clamp (определена здесь)
    int clamp(int value, int min, int max) {
        return std::max(min, std::min(max, value));
    }

    bool isValidPosition(int x, int y) const;
    bool isValidCirclePlacement(const CircleRoom& room, const std::vector<CircleRoom>& existingRooms);
    void createCircleRoom(const CircleRoom& room);
    void createWindingCorridor(int x1, int y1, int x2, int y2);
    void smoothMap(int iterations);

public:
    DungeonGenerator(std::vector<std::vector<int>>& map_ref) : map(map_ref) {
        map.resize(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, WALL));
    }

    bool generate(float& startX, float& startY, bool& switchForStart);
    void print() const;
};
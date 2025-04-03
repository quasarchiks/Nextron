#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

constexpr int TILE_SIZE = 64;
constexpr int MAP_WIDTH = 100;
constexpr int MAP_HEIGHT = 100;
constexpr int MINIMAP_TILE_SIZE = 2;

enum TileType {
    FLOOR = 0,
    WALL = 1
};
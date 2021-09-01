#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <Windows.h>

struct Parameters {
    uint32_t width;
    uint32_t height;
    uint32_t outsideWallWidth;
    uint32_t internalWallWidth;
    uint32_t splittingWallWidth;
};

enum WallOrient {
    RIGHT,
    LEFT,
    UP,
    DOWN,
};

enum WallType {
    OUTSIDE,
    INTERNAL,
    SPLITTING,
};

struct Wall {
    WallOrient orient;
    WallType type;
    uint32_t beginX;
    uint32_t beginY;
    uint32_t length;
    uint32_t startOver;
};

struct Cfg {
    Parameters param;
    std::vector<Wall> walls;

    void load ();
    void save ();
};

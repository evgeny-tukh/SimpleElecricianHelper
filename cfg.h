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
    std::string name;
};

struct Cfg {
    Parameters param;
    std::vector<Wall> walls;

    void load ();
    void save ();
};

inline const char *getWallOrientationName (WallOrient orient) {
    switch (orient) {
        case WallOrient::DOWN: return "down";
        case WallOrient::UP: return "up";
        case WallOrient::LEFT: return "left";
        case WallOrient::RIGHT: return "right";
        default: return "";
    }
}

inline const char *getWallTypeName (WallType type) {
    switch (type) {
        case WallType::INTERNAL: return "internal";
        case WallType::OUTSIDE: return "outside";
        case WallType::SPLITTING: return "splitting";
        default: return "";
    }
}

struct Ctx {
    HINSTANCE instance;
    Cfg *cfg;
    uint32_t flags;
    HPEN outsideWall, internalWall, splittingWall;
    HWND position, workspace;

    Ctx (HINSTANCE _instance, Cfg *_cfg): flags (0), instance (_instance), cfg (_cfg) {
        outsideWall = CreatePen (PS_SOLID, 10, 0);
        internalWall = CreatePen (PS_SOLID, 5, 0);
        splittingWall = CreatePen (PS_SOLID, 2, 0);
    }

    virtual ~Ctx () {
        DeleteObject (outsideWall);
        DeleteObject (internalWall);
        DeleteObject (splittingWall);
    }
};

void loadConfig (Cfg *cfg, char *filePath);
bool loadConfigFrom (HWND owner, Ctx *ctx);
void saveConfig (Cfg *cfg, char *filePath);
void saveConfigTo (HWND owner, Ctx *ctx);

extern uint32_t const NO_SKIP;

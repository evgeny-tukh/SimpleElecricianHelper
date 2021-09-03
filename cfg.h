#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <Windows.h>
#include "resource.h"

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

enum CableDir {
    GO_RIGHT,
    GO_LEFT,
    GO_UP,
    GO_DOWN,
    GO_HIGHER,
    GO_LOWER,
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

struct Cable {
    struct Node {
        uint32_t x, y, z;

        Node (uint32_t _x, uint32_t _y, uint32_t _z): x (_x), y (_y), z (_z) {}
    };

    std::vector<Node> nodes;
    std::string color, name;
    uint8_t square;
    uint8_t numOfWires;

    static uint32_t count;

    static std::string getDefNewName () {
        return std::string ("Cable ") + std::to_string (count++);
    }
    
    Cable (): color ("BLACK"), name (), numOfWires (3) {}

    void addNode (uint32_t x, uint32_t y, uint32_t z) {
        nodes.emplace_back (x, y, z);
    }

    void addNode (CableDir direction, uint32_t offset) {
        uint32_t x = nodes.back ().x;
        uint32_t y = nodes.back ().y;
        uint32_t z = nodes.back ().z;

        switch (direction) {
            case CableDir::GO_LOWER: z -= offset; break;
            case CableDir::GO_HIGHER: z += offset; break;
            case CableDir::GO_LEFT: x -= offset; break;
            case CableDir::GO_UP: y -= offset; break;
            case CableDir::GO_DOWN: y += offset; break;
            default: return;
        }

        nodes.emplace_back (x, y, z);
    }

    uint32_t calcLength () {
        return 0;
    }
};

struct Cfg {
    Parameters param;
    std::vector<Wall> walls;
    std::vector<Cable> cables;

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
    uint32_t flags, clickX, clickY;
    union {
        struct {
            HPEN outsideWall, internalWall, splittingWall;
            HPEN blackPen, redPen, greenPen, bluePen, orangePen, yellowPen, grayPen;
        };
        HPEN pens [10];
    };
    HWND position, workspace;
    HMENU contextMenu;
    Cable *curCable;

    Ctx (HINSTANCE _instance, Cfg *_cfg): flags (0), instance (_instance), cfg (_cfg), curCable (0) {
        outsideWall = CreatePen (PS_SOLID, 10, 0);
        internalWall = CreatePen (PS_SOLID, 5, 0);
        splittingWall = CreatePen (PS_SOLID, 2, 0);
        blackPen = CreatePen (PS_SOLID, 1, 0);
        redPen = CreatePen (PS_SOLID, 1, RGB (255, 0, 0));
        greenPen = CreatePen (PS_SOLID, 1, RGB (0, 255, 0));
        bluePen = CreatePen (PS_SOLID, 1, RGB (0, 0, 255));
        orangePen = CreatePen (PS_SOLID, 1, RGB (255, 165, 0));
        yellowPen = CreatePen (PS_SOLID, 1, RGB (255, 215, 0));
        grayPen = CreatePen (PS_SOLID, 1, RGB (200, 200, 200));
        contextMenu = LoadMenu (instance, MAKEINTRESOURCE (IDR_CONTEXT_MENU));
    }

    virtual ~Ctx () {
        for (auto i = 0; i < 10; DeleteObject (pens [i]++));
        DestroyMenu (contextMenu);
    }

    HPEN getColredPen (const char *color) {
        if (stricmp (color, "black") == 0) return blackPen;
        if (stricmp (color, "red") == 0) return redPen;
        if (stricmp (color, "green") == 0) return greenPen;
        if (stricmp (color, "blue") == 0) return bluePen;
        if (stricmp (color, "orange") == 0) return orangePen;
        if (stricmp (color, "yellow") == 0) return yellowPen;
        if (stricmp (color, "gray") == 0) return grayPen;
        return 0;
    }
};

void loadConfig (Cfg *cfg, char *filePath);
bool loadConfigFrom (HWND owner, Ctx *ctx);
void saveConfig (Cfg *cfg, char *filePath);
void saveConfigTo (HWND owner, Ctx *ctx);

extern uint32_t const NO_SKIP;

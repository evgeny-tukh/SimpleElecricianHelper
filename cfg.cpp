#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include <Shlwapi.h>
#include <vector>
#include <thread>
#include <memory.h>
#include "json_lite.h"
#include "cfg.h"

uint32_t const NO_SKIP = 0xFFFFFFFF;
char const *FILTER = "Configuration files (*.cfg)\0*.cfg\0All files\0*.*\0\0";

uint32_t Cable::count = 1;

bool loadConfigFrom (HWND owner, Ctx *ctx) {
    OPENFILENAME data;
    char filePath [MAX_PATH];

    memset (& data, 0, sizeof (data));
    memset (filePath, 0, sizeof (filePath));

    data.lStructSize = sizeof (data);
    data.lpstrFilter = FILTER;
    data.lpstrTitle = "Select configuration file";
    data.lpstrDefExt = ".cfg";
    data.lpstrFile = filePath;
    data.nMaxFile = sizeof (filePath);
    data.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    data.hInstance = ctx->instance;
    data.hwndOwner = owner;

    bool result = GetOpenFileName (& data);

    if (result) loadConfig (ctx->cfg, filePath);

    return result;
}

void loadConfig (Cfg *cfg, char *filePath) {
    char cfgPath [MAX_PATH];

    cfg->walls.clear ();
    cfg->cables.clear ();

    if (filePath) {
        if (!PathFileExists (filePath)) return;        
        strcpy (cfgPath, filePath);
    } else {
        GetModuleFileName (0, cfgPath, sizeof (cfgPath));
        PathRenameExtension (cfgPath, ".cfg");
    }

    if (PathFileExists (cfgPath)) {
        FILE *cfgFile = fopen (cfgPath, "rb+");

        if (cfgFile) {
            fseek (cfgFile, 0, SEEK_END);

            auto size = ftell (cfgFile);

            fseek (cfgFile, 0, SEEK_SET);

            char *buffer = (char *) malloc (size + 1);

            memset (buffer, 0, size + 1);

            if (fread (buffer, 1, size, cfgFile) > 0) {
                int next = 0;
                auto json = json::parse (buffer, buffer + size, next);

                json::hashNode *root = (json::hashNode *) json;
                json::hashNode *paramsNode = (json::hashNode *) (*root) ["params"];
                json::arrayNode *wallsNode = (json::arrayNode *) (*root) ["walls"];

                if (paramsNode != json::nothing) {
                    json::numberNode *widthNode = (json::numberNode *) (*paramsNode) ["width"];
                    json::numberNode *heightNode = (json::numberNode *) (*paramsNode) ["height"];
                    json::numberNode *outsideWallWidthNode = (json::numberNode *) (*paramsNode) ["outsideWallWidth"];
                    json::numberNode *internalWallWidthNode = (json::numberNode *) (*paramsNode) ["internalWallWidth"];
                    json::numberNode *splittingWallWidthNode = (json::numberNode *) (*paramsNode) ["splittingWallWidth"];

                    if (widthNode != json::nothing) cfg->param.width = (uint32_t) widthNode->getValue ();
                    if (heightNode != json::nothing) cfg->param.height = (uint32_t) heightNode->getValue ();
                    if (outsideWallWidthNode != json::nothing) cfg->param.outsideWallWidth = (uint32_t) outsideWallWidthNode->getValue ();
                    if (internalWallWidthNode != json::nothing) cfg->param.internalWallWidth = (uint32_t) internalWallWidthNode->getValue ();
                    if (splittingWallWidthNode != json::nothing) cfg->param.splittingWallWidth = (uint32_t) splittingWallWidthNode->getValue ();
                }

                if (wallsNode != json::nothing) {
                    for (auto item = wallsNode->begin (); item != wallsNode->end (); ++ item) {
                        json::hashNode *wallNode = (json::hashNode *) *item;
                        json::stringNode *orientNode = (json::stringNode *) (*wallNode) ["orient"];
                        json::stringNode *typeNode = (json::stringNode *) (*wallNode) ["type"];
                        json::numberNode *beginXNode = (json::numberNode *) (*wallNode) ["beginX"];
                        json::numberNode *beginYNode = (json::numberNode *) (*wallNode) ["beginY"];
                        json::numberNode *lengthNode = (json::numberNode *) (*wallNode) ["length"];
                        json::numberNode *startOverNode = (json::numberNode *) (*wallNode) ["startOver"];
                        json::stringNode *nameNode = (json::stringNode *) (*wallNode) ["name"];

                        cfg->walls.emplace_back ();

                        auto& wall = cfg->walls.back ();

                        if (nameNode != json::nothing) wall.name = nameNode->getValue ();

                        if (orientNode == json::nothing) {
                            wall.orient = WallOrient::RIGHT;
                        } else {
                            auto orient = orientNode->getValue ();

                            if (stricmp (orient, "left") == 0) {
                                wall.orient = WallOrient::LEFT;
                            } else if (stricmp (orient, "right") == 0) {
                                wall.orient = WallOrient::RIGHT;
                            } else if (stricmp (orient, "up") == 0) {
                                wall.orient = WallOrient::UP;
                            } else if (stricmp (orient, "down") == 0) {
                                wall.orient = WallOrient::DOWN;
                            } else {
                                wall.orient = WallOrient::RIGHT;
                            }
                        }

                        if (typeNode == json::nothing) {
                            wall.type = WallType::INTERNAL;
                        } else {
                            auto type = typeNode->getValue ();

                            if (stricmp (type, "internal") == 0) {
                                wall.type = WallType::INTERNAL;
                            } else if (stricmp (type, "outside") == 0) {
                                wall.type = WallType::OUTSIDE;
                            } else if (stricmp (type, "splitting") == 0) {
                                wall.type = WallType::SPLITTING;
                            } else {
                                wall.type = WallType::INTERNAL;
                            }
                        }

                        wall.beginX = (beginXNode != json::nothing) ? (uint32_t) beginXNode->getValue () : 0;
                        wall.beginY = (beginYNode != json::nothing) ? (uint32_t) beginYNode->getValue () : 0;
                        wall.length = (lengthNode != json::nothing) ? (uint32_t) lengthNode->getValue () : 0;
                        wall.startOver = (startOverNode == json::nothing) ? NO_SKIP : (uint32_t) startOverNode->getValue ();
                    }
                }

                delete json;
                free (buffer);
            }

            fclose (cfgFile);
        }
    }
}

void saveConfigTo (HWND owner, Ctx *ctx) {
    OPENFILENAME data;
    char filePath [MAX_PATH];

    memset (& data, 0, sizeof (data));
    memset (filePath, 0, sizeof (filePath));

    data.lStructSize = sizeof (data);
    data.lpstrFilter = FILTER;
    data.lpstrTitle = "Select file to save";
    data.lpstrDefExt = ".cfg";
    data.lpstrFile = filePath;
    data.nMaxFile = sizeof (filePath);
    data.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    data.hInstance = ctx->instance;
    data.hwndOwner = owner;

    bool result = GetSaveFileName (& data);

    if (result) saveConfig (ctx->cfg, filePath);
}

void saveConfig (Cfg *cfg, char *filePath) {
    char cfgPath [MAX_PATH];

    if (filePath) {
        strcpy (cfgPath, filePath);
    } else {
        GetModuleFileName (0, cfgPath, sizeof (cfgPath));
        PathRenameExtension (cfgPath, ".cfg");
    }

    json::hashNode body;
    json::hashNode *paramsNode = new json::hashNode;
    json::arrayNode *wallsNode = new json::arrayNode;

    body.add ("params", paramsNode);
    body.add ("walls", wallsNode);

    paramsNode->add ("width", new json::numberNode (cfg->param.width));
    paramsNode->add ("height", new json::numberNode (cfg->param.height));
    paramsNode->add ("outsideWallWidth", new json::numberNode (cfg->param.outsideWallWidth));
    paramsNode->add ("internalWallWidth", new json::numberNode (cfg->param.internalWallWidth));
    paramsNode->add ("splittingWallWidth", new json::numberNode (cfg->param.splittingWallWidth));

    for (auto& wall: cfg->walls) {
        json::hashNode *wallNode = new json::hashNode;

        wallNode->add ("name", new json::stringNode (wall.name.c_str ()));
        wallNode->add ("orient", new json::stringNode (getWallOrientationName (wall.orient)));
        wallNode->add ("type", new json::stringNode (getWallTypeName (wall.type)));
        wallNode->add ("length", new json::numberNode (wall.length));

        if (wall.startOver == NO_SKIP) {
            wallNode->add ("beginX", new json::numberNode (wall.beginX));
            wallNode->add ("beginY", new json::numberNode (wall.beginY));
        } else {
            wallNode->add ("startOver", new json::numberNode (wall.startOver));
        }

        wallsNode->add (wallNode);
    }

    std::string buffer = body.serialize ();
    
    FILE *cfgFile = fopen (cfgPath, "wb");

    if (cfgFile) {
        fwrite (buffer.c_str (), 1, buffer.length (), cfgFile);
        fclose (cfgFile);
    }
}

void Cable::addNode (CableDir direction, uint32_t offset) {
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

Ctx::Ctx (HINSTANCE _instance, Cfg *_cfg):
    flags (0),
    instance (_instance),
    cfg (_cfg),
    curCable (0),
    curX (0),
    curY (0),
    clickX (0),
    clickY (0),
    prevX (0),
    prevY (0),
    selectedX (0),
    selectedY (0) {
    outsideWall = CreatePen (PS_SOLID, 10, 0);
    internalWall = CreatePen (PS_SOLID, 5, 0);
    splittingWall = CreatePen (PS_SOLID, 2, 0);
    blackPen = CreatePen (PS_SOLID, 2, 0);
    redPen = CreatePen (PS_SOLID, 2, RGB (255, 0, 0));
    greenPen = CreatePen (PS_SOLID, 2, RGB (0, 255, 0));
    bluePen = CreatePen (PS_SOLID, 2, RGB (0, 0, 255));
    orangePen = CreatePen (PS_SOLID, 2, RGB (255, 165, 0));
    yellowPen = CreatePen (PS_SOLID, 2, RGB (255, 215, 0));
    grayPen = CreatePen (PS_SOLID, 2, RGB (200, 200, 200));
    dashedPen = CreatePen (PS_DASH, 1, RGB (0, 0, 255));
    contextMenu = LoadMenu (instance, MAKEINTRESOURCE (IDR_CONTEXT_MENU));
}

HPEN Ctx::getColoredPen (const char *color) {
    if (stricmp (color, "black") == 0) return blackPen;
    if (stricmp (color, "red") == 0) return redPen;
    if (stricmp (color, "green") == 0) return greenPen;
    if (stricmp (color, "blue") == 0) return bluePen;
    if (stricmp (color, "orange") == 0) return orangePen;
    if (stricmp (color, "yellow") == 0) return yellowPen;
    if (stricmp (color, "gray") == 0) return grayPen;
    return 0;
}

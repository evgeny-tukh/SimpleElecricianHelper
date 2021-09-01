#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include <Shlwapi.h>
#include <vector>
#include <thread>
#include <memory.h>
#include "json_lite.h"
#include "resource.h"
#include "cfg.h"

struct Ctx {
    HINSTANCE instance;
    Cfg *cfg;
    uint32_t flags;
    HPEN outsideWall, internalWall, splittingWall;

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

char const *CLS_NAME = "ehWin";
uint32_t const NO_SKIP = 0xFFFFFFFF;

void initWindow (HWND wnd, void *data) {
    Ctx *ctx = (Ctx *) data;
    RECT client;

    GetClientRect (wnd, & client);

    auto createControl = [&wnd, &ctx] (const char *className, const char *text, uint32_t style, bool visible, int x, int y, int width, int height, uint64_t id) {
        style |= WS_CHILD;

        if (visible) style |= WS_VISIBLE;
        
        CreateWindow (className, text, style, x, y, width, height, wnd, (HMENU) id, ctx->instance, 0);
    };

    SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) data);
}

void doCommand (HWND wnd, uint16_t command) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
}

void paintWindow (HWND wnd) {
    PAINTSTRUCT data;
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    HDC paintCtx = BeginPaint (wnd, & data);
    RECT client;
    uint32_t lastX, lastY;
    static const uint32_t EDGE = 50;
    auto project = [&client, &ctx] (POINT& pos, POINT& clientPos) {
        clientPos.x = EDGE + (uint32_t) (double) pos.x * (double) (client.right - EDGE * 2) / (double) ctx->cfg->param.width;
        clientPos.y = EDGE + (uint32_t) (double) pos.y * (double) (client.bottom - EDGE * 2) / (double) ctx->cfg->param.height;
    };
    auto drawBox = [project, paintCtx] (uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        POINT tl { (LONG) x, (LONG) y };
        POINT br { (LONG) (x + width - 1), (LONG) (y + height - 1) };
        POINT clientTl, clientBr;

        project (tl, clientTl);
        project (br, clientBr);

        SelectObject (paintCtx, GetStockObject (BLACK_PEN));
        Rectangle (paintCtx, clientTl.x, clientTl.y, clientBr.x, clientBr.y);
    };
    auto drawLine = [project, paintCtx, ctx, &lastX, &lastY] (Wall& wall) {
        POINT begin, end, clientBegin, clientEnd;

        if (wall.startOver != NO_SKIP) {
            switch (wall.orient) {
                case WallOrient::DOWN: {
                    begin.x = lastX;
                    begin.y = lastY + wall.startOver;
                    break;
                }
                case WallOrient::UP: {
                    begin.x = lastX;
                    begin.y = lastY - wall.startOver;
                    break;
                }
                case WallOrient::RIGHT: {
                    begin.x = lastX + wall.startOver;
                    begin.y = lastY;
                    break;
                }
                case WallOrient::LEFT: {
                    begin.x = lastX - wall.startOver;
                    begin.y = lastY;
                    break;
                }

                default: return;
            }
        } else {
            begin.x = wall.beginX;
            begin.y = wall.beginY;
        }

        end.x = begin.x;
        end.y = begin.y;

        switch (wall.orient) {
            case WallOrient::DOWN: end.y += wall.length; break;
            case WallOrient::UP: end.y -= wall.length; break;
            case WallOrient::RIGHT: end.x += wall.length; break;
            case WallOrient::LEFT: end.x -= wall.length; break;
            default: return;
        }

        int x1, x2, y1, y2, wallWidth;

        switch (wall.type) {
            case WallType::INTERNAL: wallWidth = ctx->cfg->param.internalWallWidth; break;
            case WallType::OUTSIDE: wallWidth = ctx->cfg->param.outsideWallWidth; break;
            case WallType::SPLITTING: wallWidth = ctx->cfg->param.splittingWallWidth; break;
            default: return;
        }

        switch (wall.orient) {
            case WallOrient::DOWN: {
                x1 = begin.x;
                y1 = begin.y;
                x2 = begin.x + wallWidth;
                y2 = begin.y + wall.length;
                lastX = x1;
                lastY = y2;
                break;
            }
            case WallOrient::UP: {
                x1 = begin.x;
                y1 = begin.y;
                x2 = begin.x + wallWidth;
                y2 = begin.y - wall.length;
                lastX = x1;
                lastY = y2;
                break;
            }
            case WallOrient::RIGHT: {
                x1 = begin.x;
                y1 = begin.y;
                x2 = begin.x + wall.length;
                y2 = begin.y + wallWidth;
                lastY = y1;
                lastX = x2;
                break;
            }
            case WallOrient::LEFT: {
                x1 = begin.x;
                y1 = begin.y;
                x2 = begin.x - wall.length;
                y2 = begin.y + wallWidth;
                lastY = y1;
                lastX = x2;
                break;
            }
        }

        begin.x = x1;
        begin.y = y1;
        end.x = x2;
        end.y = y2;

        project (begin, clientBegin);
        project (end, clientEnd);

        RECT wallRect;

        wallRect.left = min (clientBegin.x, clientEnd.x);
        wallRect.right = max (clientBegin.x, clientEnd.x);
        wallRect.top = min (clientBegin.y, clientEnd.y);
        wallRect.bottom = max (clientBegin.y, clientEnd.y);

        SelectObject (paintCtx, GetStockObject (BLACK_PEN));
        FillRect (paintCtx, & wallRect, (HBRUSH) GetStockObject (BLACK_BRUSH));
        /*switch (wall.type) {
            case WallType::INTERNAL: SelectObject (paintCtx, ctx->internalWall); break;
            case WallType::OUTSIDE: SelectObject (paintCtx, ctx->outsideWall); break;
            case WallType::SPLITTING: SelectObject (paintCtx, ctx->splittingWall); break;
            default: SelectObject (paintCtx, GetStockObject (BLACK_PEN));
        }

        MoveToEx (paintCtx, clientBegin.x, clientBegin.y, 0);
        LineTo (paintCtx, clientEnd.x, clientEnd.y);

        lastX = end.x;
        lastY = end.y;*/
    };

    GetClientRect (wnd, & client);
    drawBox (0, 0, ctx->cfg->param.width, ctx->cfg->param.height);

    for (auto& wall: ctx->cfg->walls) drawLine (wall);

    /*HDC tempCtx = CreateCompatibleDC (paintCtx);

    GetClientRect (wnd, & client);
    SelectObject (paintCtx, GetStockObject (BLACK_PEN));
    Rectangle (paintCtx, 9, 9, 311, client.bottom - 9);
    SelectObject (tempCtx, ctx->image);
    BitBlt (paintCtx, 10, 10, 300, client.bottom - 20, tempCtx, 300, 100, SRCCOPY);
    SelectObject (tempCtx, (HBITMAP) 0);
    DeleteDC (tempCtx);*/
    EndPaint (wnd, & data);
}

bool queryExit (HWND wnd) {
    return MessageBox (wnd, "Do you want to quit the application?", "Confirmation", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

LRESULT wndProc (HWND wnd, UINT msg, WPARAM param1, LPARAM param2) {
    LRESULT result = 0;

    switch (msg) {
        case WM_PAINT:
            paintWindow (wnd); break;
        case WM_COMMAND:
            doCommand (wnd, LOWORD (param1)); break;
        case WM_SYSCOMMAND: {
            if (param1 == SC_CLOSE && queryExit (wnd)) DestroyWindow (wnd);
            break;
        }
        case WM_CREATE:
            initWindow (wnd, ((CREATESTRUCT *) param2)->lpCreateParams); break;
        case WM_DESTROY:
            PostQuitMessage (9); break;
        default:
            result = DefWindowProc (wnd, msg, param1, param2);
    }
    
    return result;
}

void registerClass (HINSTANCE instance) {
    WNDCLASS classInfo;

    memset (&classInfo, 0, sizeof (classInfo));

    classInfo.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    classInfo.hCursor = (HCURSOR) LoadCursor (0, IDC_ARROW);
    classInfo.hIcon = (HICON) LoadIcon (0, IDI_APPLICATION);
    classInfo.hInstance = instance;
    classInfo.lpfnWndProc = wndProc;
    classInfo.lpszClassName = CLS_NAME;
    classInfo.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClass (&classInfo);
}

void initCommonControls () {
    INITCOMMONCONTROLSEX data;
	
    data.dwSize = sizeof (INITCOMMONCONTROLSEX);
	data.dwICC = ICC_WIN95_CLASSES;
	
    InitCommonControlsEx (& data);
}

void loadConfig (Cfg& cfg) {
    char cfgPath [MAX_PATH];

    GetModuleFileName (0, cfgPath, sizeof (cfgPath));
    PathRenameExtension (cfgPath, ".cfg");

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

                    if (widthNode != json::nothing) cfg.param.width = (uint32_t) widthNode->getValue ();
                    if (heightNode != json::nothing) cfg.param.height = (uint32_t) heightNode->getValue ();
                    if (outsideWallWidthNode != json::nothing) cfg.param.outsideWallWidth = (uint32_t) outsideWallWidthNode->getValue ();
                    if (internalWallWidthNode != json::nothing) cfg.param.internalWallWidth = (uint32_t) internalWallWidthNode->getValue ();
                    if (splittingWallWidthNode != json::nothing) cfg.param.splittingWallWidth = (uint32_t) splittingWallWidthNode->getValue ();
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

                        cfg.walls.emplace_back ();

                        auto& wall = cfg.walls.back ();

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

int APIENTRY WinMain (HINSTANCE instance, HINSTANCE prev, char *cmdLine, int showCmd) {
    Cfg cfg;
    Ctx ctx (instance, & cfg);

    CoInitialize (0);
    initCommonControls ();
    registerClass (instance);
    loadConfig (cfg);

    auto mainWnd = CreateWindow (
        CLS_NAME,
        "Simple Electrician Helper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        600,
        0,
        0,
        instance,
        & ctx
    );

    ShowWindow (mainWnd, SW_SHOW);
    UpdateWindow (mainWnd);

    MSG msg;

    while (GetMessage (&msg, 0, 0, 0)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
}
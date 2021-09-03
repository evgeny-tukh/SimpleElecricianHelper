#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include <Shlwapi.h>
#include <vector>
#include <thread>
#include <memory.h>
#include "resource.h"
#include "cfg.h"
#include "editbox.h"
#include "cable_props.h"

enum NodeAddMode {
    HERE,
    SAME_X,
    SAME_Y,
};

char const *CLS_NAME = "ehWin";
char const *WSP_CLS_NAME = "ehWinWsp";
uint32_t const EDGE = 50;

bool screenToReal (HWND wnd, int screenX, int screenY, int& realX, int& realY);

bool queryExit (HWND wnd) {
    return MessageBox (wnd, "Do you want to quit the application?", "Confirmation", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

void initWorkspace (HWND wnd, void *data) {
    Ctx *ctx = (Ctx *) data;
    SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) data);
}

void initWindow (HWND wnd, void *data) {
    Ctx *ctx = (Ctx *) data;
    RECT client;

    GetClientRect (wnd, & client);

    auto createControl = [&wnd, &ctx] (const char *className, const char *text, uint32_t style, bool visible, int x, int y, int width, int height, uint64_t id) {
        style |= WS_CHILD;

        if (visible) style |= WS_VISIBLE;
        
        return CreateWindow (className, text, style, x, y, width, height, wnd, (HMENU) id, ctx->instance, 0);
    };
    auto createPopup = [&wnd, &ctx] (const char *className, const char *text, uint32_t style, bool visible, int x, int y, int width, int height) {
        style |= WS_POPUP;

        if (visible) style |= WS_VISIBLE;
        
        return CreateWindow (className, text, style, x, y, width, height, wnd, 0, ctx->instance, 0);
    };

    SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) data);

    //ctx->position = createPopup ("STATIC", "N/A", SS_CENTER | WS_BORDER, true, CW_USEDEFAULT, CW_USEDEFAULT, 100, 30);
    ctx->workspace = CreateWindow (WSP_CLS_NAME, "", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 21, client.right, client.bottom - 22, wnd, 0, ctx->instance, ctx);
    ctx->position = CreateWindow ("STATIC", "N/A", WS_CHILD | SS_CENTER | WS_BORDER | WS_VISIBLE, 0, 0, 400, 22, wnd, 0, ctx->instance, 0);
}

void cancelCable (HWND wnd) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    if (ctx->curCable) {
        delete ctx->curCable;
        ctx->curCable = 0;
    }
}

void beginCable (HWND wnd) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    int realX, realY;

    if (!screenToReal (wnd, ctx->clickX, ctx->clickY, realX, realY)) return;

    Cable *cable = new Cable;

    uint32_t x = realX;
    uint32_t y = realY;
    uint32_t z = 1000;
    uint32_t numOfWires = 3;
    std::string color ("BLACK");
    std::string name (Cable::getDefNewName ());

    if (getNewCableProps (wnd, ctx->instance, numOfWires, x, y, z, name, color)) {
        ctx->curCable = new Cable;
        ctx->curCable->name = name;
        ctx->curCable->color = color;
        ctx->curCable->addNode (x, y, z);
    } else {
        delete cable;
    }
}

void addNewCableNode (HWND wnd, NodeAddMode mode) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    int realX, realY;

    if (!ctx->curCable || !screenToReal (wnd, ctx->clickX, ctx->clickY, realX, realY)) return;

    auto& lastNode = ctx->curCable->nodes.back ();

    uint32_t x, y, z;

    switch (mode) {
        case NodeAddMode::HERE:
            x = realX;
            y = realY;
            z = lastNode.z;
            break;
        case NodeAddMode::SAME_X:
            x = lastNode.x;
            y = realY;
            z = lastNode.z;
            break;
        case NodeAddMode::SAME_Y:
            x = realX;
            y = lastNode.y;
            z = lastNode.z;
            break;
        default:
            return;
    }

    ctx->curCable->addNode (x, y, z);
}

void doCommand (HWND wnd, uint16_t command) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    switch (command) {
        case ID_ADD_CABLE_NODE_HERE:
            addNewCableNode (wnd, NodeAddMode::HERE);
            InvalidateRect (ctx->workspace, 0, 1);
            break;
        case ID_EXIT:
            if (queryExit (wnd)) DestroyWindow (wnd);
            break;
        case ID_LOAD:
            if (loadConfigFrom (wnd, ctx)) InvalidateRect (wnd, 0, 1);
            break;
        case ID_SAVE:
            saveConfigTo (wnd, ctx); break;
        case ID_BEGIN_CABLE:
            beginCable (wnd); break;
        case ID_CANCEL_CABLE:
            if (MessageBox (wnd, "Cancel cable", "Do you want to cancel the editing cable?", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                cancelCable (wnd);
            }
            break;
    }
}

void paintWorkspace (HWND wnd) {
    PAINTSTRUCT data;
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    HDC paintCtx = BeginPaint (wnd, & data);
    RECT client;
    uint32_t lastX, lastY;
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
    auto drawCable = [project, paintCtx, ctx] (Cable& cable) {
        SelectObject (paintCtx, ctx->getColredPen (cable.color.c_str ()));
        POINT realPos, screenPos;
        realPos.x = cable.nodes.front ().x;
        realPos.y = cable.nodes.front ().y;
        project (realPos, screenPos);
        MoveToEx (paintCtx, screenPos.x, screenPos.y, 0);
        
        for (auto i = 1; i < cable.nodes.size (); ++ i) {
            realPos.x = cable.nodes [i].x;
            realPos.y = cable.nodes [i].y;
            project (realPos, screenPos);
            LineTo (paintCtx, screenPos.x, screenPos.y);
        }
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
    };

    RECT workingArea;
    GetClientRect (wnd, & client);
    GetClientRect (wnd, & workingArea);
    workingArea.left += EDGE;
    workingArea.top += EDGE;
    workingArea.right -= EDGE;
    workingArea.bottom -= EDGE;

    FillRect (paintCtx, & workingArea, (HBRUSH) GetStockObject (GRAY_BRUSH));        
    drawBox (0, 0, ctx->cfg->param.width, ctx->cfg->param.height);

    for (auto& wall: ctx->cfg->walls) drawLine (wall);
    for (auto& cable: ctx->cfg->cables) drawCable (cable);

    if (ctx->curCable) drawCable (*(ctx->curCable));

    EndPaint (wnd, & data);
}

void onSize (HWND wnd, uint16_t width, uint16_t height) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    MoveWindow (ctx->workspace, 0, 22, width, height - 22, true);
}

LRESULT wndProc (HWND wnd, UINT msg, WPARAM param1, LPARAM param2) {
    LRESULT result = 0;

    switch (msg) {
        case WM_SIZE:
            onSize (wnd, LOWORD (param2), HIWORD (param2)); break;
        case WM_COMMAND:
            doCommand (wnd, LOWORD (param1)); break;
        case WM_CREATE:
            initWindow (wnd, ((CREATESTRUCT *) param2)->lpCreateParams); break;
        case WM_DESTROY:
            PostQuitMessage (9); break;
        case WM_SYSCOMMAND:
            if (param1 == SC_CLOSE && queryExit (wnd)) DestroyWindow (wnd);
        default:
            result = DefWindowProc (wnd, msg, param1, param2);
    }
    
    return result;
}

bool screenToReal (HWND wnd, int screenX, int screenY, int& realX, int& realY) {
    RECT client;

    GetClientRect (wnd, & client);

    if (screenX < EDGE || screenY < EDGE || (client.right - screenX) < EDGE || (client.bottom - screenY) < EDGE) {
        return false;
    } else {
        Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

        realX = (int) ((double) (screenX - EDGE) * (double) ctx->cfg->param.width / (double) (client.right - EDGE * 2));
        realY = (int) ((double) (screenY - EDGE) * (double) ctx->cfg->param.height / (double) (client.bottom - EDGE * 2));

        return true;
    }
}

void onMouseMove (HWND wnd, int clientX, int clientY) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    int realX, realY;

    if (screenToReal (wnd, clientX, clientY, realX, realY)) {
        char msg [100];
        sprintf (msg, "X: %d; Y: %d", realX, realY);

        SetWindowText (ctx->position, msg);
    } else {
        SetWindowText (ctx->position, "");
    }
}

void onRightButtonDown (HWND wnd, int x, int y) {
    Ctx *ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    RECT window;

    GetWindowRect (wnd, & window);

    ctx->clickX = x + window.left;
    ctx->clickY = y + window.top;

    TrackPopupMenu (GetSubMenu (ctx->contextMenu, 0), TPM_LEFTALIGN | TPM_TOPALIGN, ctx->clickX, ctx->clickY, 0, GetParent (wnd), 0);
}

LRESULT workspaceWndProc (HWND wnd, UINT msg, WPARAM param1, LPARAM param2) {
    LRESULT result = 0;

    switch (msg) {
        case WM_RBUTTONDOWN:
            onRightButtonDown (wnd, LOWORD (param2), HIWORD (param2)); break;
        case WM_MOUSEMOVE:
            onMouseMove (wnd, LOWORD (param2), HIWORD (param2)); break;
        case WM_PAINT:
            paintWorkspace (wnd); break;
        case WM_CREATE:
            initWorkspace (wnd, ((CREATESTRUCT *) param2)->lpCreateParams); break;
        default:
            result = DefWindowProc (wnd, msg, param1, param2);
    }
    
    return result;
}

void registerClass (HINSTANCE instance) {
    WNDCLASS classInfo;

    memset (& classInfo, 0, sizeof (classInfo));

    classInfo.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    classInfo.hCursor = (HCURSOR) LoadCursor (0, IDC_ARROW);
    classInfo.hIcon = (HICON) LoadIcon (0, IDI_APPLICATION);
    classInfo.hInstance = instance;
    classInfo.lpfnWndProc = wndProc;
    classInfo.lpszClassName = CLS_NAME;
    classInfo.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClass (& classInfo);

    classInfo.lpfnWndProc = workspaceWndProc;
    classInfo.lpszClassName = WSP_CLS_NAME;

    RegisterClass (& classInfo);
}

void initCommonControls () {
    INITCOMMONCONTROLSEX data;
	
    data.dwSize = sizeof (INITCOMMONCONTROLSEX);
	data.dwICC = ICC_WIN95_CLASSES;
	
    InitCommonControlsEx (& data);
}

int APIENTRY WinMain (HINSTANCE instance, HINSTANCE prev, char *cmdLine, int showCmd) {
    Cfg cfg;
    Ctx ctx (instance, & cfg);

    CoInitialize (0);
    initCommonControls ();
    registerClass (instance);
    
    if (cmdLine && *cmdLine) loadConfig (& cfg, cmdLine);

    auto mainWnd = CreateWindow (
        CLS_NAME,
        "Simple Electrician Helper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        600,
        0,
        LoadMenu (instance, MAKEINTRESOURCE (IDR_MAINMENU)),
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
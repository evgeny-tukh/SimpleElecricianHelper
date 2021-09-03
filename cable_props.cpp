#include <Windows.h>
#include <cstdint>
#include "resource.h"
#include "cfg.h"

struct CableCtx {
    uint8_t numOfWires;
    uint32_t x, y, z;
    std::string name, color;
};

INT_PTR cableEditProc (HWND wnd, UINT msg, WPARAM param1, LPARAM param2) {
    switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD (param1)) {
                case ID_OK: {
                    CableCtx *ctx = (CableCtx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
                    char buffer [100];
                    GetDlgItemText (wnd, ID_COLOR, buffer, sizeof (buffer));
                    ctx->color = buffer;
                    GetDlgItemText (wnd, ID_NAME, buffer, sizeof (buffer));
                    ctx->name = buffer;
                    ctx->numOfWires = GetDlgItemInt (wnd, ID_NUM_OF_WIRES, 0, 0);
                    int z = GetDlgItemInt (wnd, ID_ELEVATION, 0, 0);
                    EndDialog (wnd, ID_OK);
                    return 1;
                }
                case ID_CANCEL: {
                    EndDialog (wnd, ID_CANCEL); return 1;
                }
            }
            return 1;
        }
        case WM_INITDIALOG: {
            CableCtx *ctx = (CableCtx *) param2;

            SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) param2);
            SetDlgItemText (wnd, ID_ELEVATION, "1000");
            SetDlgItemInt (wnd, ID_NUM_OF_WIRES, ctx->numOfWires, 0);
            SetDlgItemText (wnd, ID_NAME, ctx->name.c_str ());
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "BLACK");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "RED");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "GREEN");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "BLUE");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "ORANGE");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "YELLOW");
            SendDlgItemMessage (wnd, ID_COLOR, CB_ADDSTRING, 0, (LPARAM) "GRAY");
            SendDlgItemMessage (wnd, ID_COLOR, CB_SETCURSEL, 0, 0);
            return 1;
        }
        default: {
            return 0;
        }
    }
}

bool getNewCableProps (
    HWND parent,
    HINSTANCE instance,
    uint8_t numOfWires,
    uint32_t& x,
    uint32_t& y,
    uint32_t& z,
    std::string& name,
    std::string& color
) {
    CableCtx ctx { numOfWires, x, y, z, name, color };

    bool result = DialogBoxParam (instance, MAKEINTRESOURCE (IDD_CABLE_PROPS), parent, cableEditProc, (LPARAM) & ctx) == ID_OK;

    if (result) {
        numOfWires = ctx.numOfWires;
        x = ctx.x;
        y = ctx.y;
        z = ctx.z;
        name = ctx.name;
        color = ctx.color;
    }

    return result;
}


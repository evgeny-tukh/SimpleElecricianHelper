#pragma once
#include <cstdint>
#include <windows.h>
#include "cfg.h"

bool getNewCableProps (
    HWND parent,
    HINSTANCE instance,
    uint8_t numOfWires,
    uint32_t& x,
    uint32_t& y,
    uint32_t& z,
    std::string& name,
    std::string& color
);

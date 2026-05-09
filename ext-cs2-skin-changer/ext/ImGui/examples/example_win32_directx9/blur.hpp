#pragma once
#include "imgui.h"

// Blur stub - original implementation uses D3D9, this project uses D3D11.
// draw_blur is defined as a no-op to maintain compatibility.
inline void draw_blur( ImDrawList* drawList ) {
    // no-op: blur not supported on D3D11 backend
}

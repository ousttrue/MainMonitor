#pragma once
#include <ScreenState.h>

namespace gui
{

struct ViewValue
{
    float clearColor[4] = {0, 0, 0, 1};
    bool showGrid = true;
    bool showGizmo = true;
    bool showVR = false;
};

bool View(void *view, const screenstate::ScreenState &state, size_t textureID,
          screenstate::ScreenState *viewState, ViewValue *value);

} // namespace gui

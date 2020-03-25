#pragma once
#include <ScreenState.h>
#include <hierarchy.h>

namespace gui
{

bool View(hierarchy::SceneView *view, const screenstate::ScreenState &state, size_t textureID,
          screenstate::ScreenState *viewState);

} // namespace gui

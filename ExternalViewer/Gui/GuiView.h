#pragma once
#include <ScreenState.h>

namespace gui
{

bool View(void *view, const screenstate::ScreenState &state, size_t textureID, screenstate::ScreenState *viewState);

}

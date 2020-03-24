#pragma once
#include <hierarchy.h>
#include <ScreenState.h>

namespace gui
{

class Gui
{
    class GuiImpl *m_impl = nullptr;

public:
    Gui();
    ~Gui();
    void Log(const char *msg);
    void OnFrame(const screenstate::ScreenState &state, hierarchy::Scene *scene);
    bool View(void *view, const screenstate::ScreenState &state, size_t textureID,
              screenstate::ScreenState *viewState, struct ViewValue *value);
};

} // namespace gui

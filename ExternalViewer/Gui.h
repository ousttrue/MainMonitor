#pragma once
#include <hierarchy.h>
#include <ScreenState.h>

class Gui
{
    class GuiImpl *m_impl = nullptr;

public:
    Gui();
    ~Gui();
    void Log(const char *msg);
    hierarchy::SceneNodePtr Selected() const;
    void NewFrame(const screenstate::ScreenState &state, hierarchy::Scene *scene);
    bool View(const screenstate::ScreenState &state, size_t textureID, screenstate::ScreenState *viewState);
};

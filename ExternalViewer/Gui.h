#pragma once
#include <hierarchy.h>
#include <ScreenState.h>

#include <list>
#include <memory>
#include <mutex>


class Gui
{
    std::unique_ptr<struct ExampleAppLog> m_logger;

    // single selection
    std::weak_ptr<hierarchy::SceneNode> m_selected;

public:
    Gui();
    ~Gui();

    void NewFrame(const screenstate::ScreenState &state);
    bool View(const screenstate::ScreenState &state, size_t textureID, screenstate::ScreenState *viewState);
    bool Update(hierarchy::Scene *scene, float clearColor[4]);

    hierarchy::SceneNodePtr Selected() const
    {
        return m_selected.lock();
    }

    void Log(const char *msg);
    void ShowLogger();

private:
    void DrawNode(const hierarchy::SceneNodePtr &node, hierarchy::SceneNode *selected);
};

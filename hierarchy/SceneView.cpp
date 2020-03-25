#include "SceneView.h"
#include "Scene.h"
#include <functional>

namespace hierarchy
{

static void UpdateDrawListIf(SceneView *view, const Scene *scene, const std::function<bool(const SceneMaterialPtr &)> filter)
{
    if (view->ShowGrid)
    {
        for (auto &node : scene->gizmoNodes)
        {
            node->UpdateWorld();
            view->Drawlist.Traverse(node);
        }
    }
    if (view->ShowVR)
    {
        for (auto &node : scene->vrNodes)
        {
            node->UpdateWorld();
            view->Drawlist.Traverse(node);
        }
    }
    for (auto &node : scene->sceneNodes)
    {
        node->UpdateWorld();
        view->Drawlist.Traverse(node);
    }
}

void SceneView::UpdateDrawList(const Scene *scene)
{
    Drawlist.Clear();
    UpdateDrawListIf(this, scene, [](const SceneMaterialPtr &m) {
        return false;
    });
}

}
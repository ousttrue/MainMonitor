#include "Scene.h"

namespace hierarchy
{

Scene::Scene()
{
}

void Scene::UpdateDrawList(const SceneViewPtr &view)
{
    drawlist.Clear();
    if (view->ShowGrid)
    {
        for (auto &node : gizmoNodes)
        {
            node->UpdateWorld();
            drawlist.Traverse(node);
        }
    }
    if (view->ShowVR)
    {
        for (auto &node : vrNodes)
        {
            node->UpdateWorld();
            drawlist.Traverse(node);
        }
    }
    for (auto &node : sceneNodes)
    {
        node->UpdateWorld();
        drawlist.Traverse(node);
    }
}

} // namespace hierarchy

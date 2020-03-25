#include "Scene.h"
#include "SceneNode.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"
#include <functional>

namespace hierarchy
{

Scene::Scene()
{
}

static void UpdateRecursive(const SceneNodePtr &node)
{
    auto mesh = node->Mesh();
    if (mesh)
    {
        auto skin = mesh->skin;
        if (skin)
        {
            // update matrix
            auto &vertices = mesh->vertices;
            skin->Update(vertices->buffer.data(), vertices->stride, vertices->Count());
        }
    }

    int count;
    auto child = node->GetChildren(&count);
    for (int i = 0; i < count; ++i, ++child)
    {
        UpdateRecursive(*child);
    }
}

void Scene::Update()
{
    for (auto &node : gizmoNodes)
    {
        node->UpdateWorld();
        UpdateRecursive(node);
    }
    for (auto &node : vrNodes)
    {
        node->UpdateWorld();
        UpdateRecursive(node);
    }
    for (auto &node : sceneNodes)
    {
        node->UpdateWorld();
        UpdateRecursive(node);
    }
}

} // namespace hierarchy

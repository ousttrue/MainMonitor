#include "SceneView.h"
#include "Scene.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"
#include <functional>

namespace hierarchy
{
using FilterFunc = std::function<bool(const SceneMaterialPtr &)>;

void Traverse(DrawList *drawlist, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
{
    auto mesh = node->Mesh();
    if (mesh)
    {
        drawlist->Nodes.push_back({
            .NodeID = node->ID(),
            .WorldMatrix = node->World().RowMatrix(),
        });
        drawlist->Meshes.push_back({
            .NodeID = node->ID(),
            .Mesh = mesh,
        });

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
        Traverse(drawlist, *child, filter);
    }
}

static void UpdateDrawListIf(SceneView *view, const Scene *scene, const FilterFunc &filter)
{
    if (view->ShowGrid)
    {
        for (auto &node : scene->gizmoNodes)
        {
            node->UpdateWorld();
            Traverse(&view->Drawlist, node, filter);
        }
    }
    if (view->ShowVR)
    {
        for (auto &node : scene->vrNodes)
        {
            node->UpdateWorld();
            Traverse(&view->Drawlist, node, filter);
        }
    }
    for (auto &node : scene->sceneNodes)
    {
        node->UpdateWorld();
        Traverse(&view->Drawlist, node, filter);
    }
}

void SceneView::UpdateDrawList(const Scene *scene)
{
    Drawlist.Clear();
    // Opaque
    UpdateDrawListIf(this, scene, [](const SceneMaterialPtr &m) {
        return m->alphaMode != AlphaMode::Blend;
    });
    // AlphaBlend
    UpdateDrawListIf(this, scene, [](const SceneMaterialPtr &m) {
        return m->alphaMode == AlphaMode::Blend;
    });
}

} // namespace hierarchy
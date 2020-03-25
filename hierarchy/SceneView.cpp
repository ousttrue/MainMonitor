#include "SceneView.h"
#include "Scene.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"
#include <functional>

namespace hierarchy
{
using FilterFunc = std::function<bool(const SceneMaterialPtr &)>;

void TraverseNode(DrawList *drawlist, const std::shared_ptr<SceneNode> &node)
{
    drawlist->Nodes.push_back({
        .NodeID = node->ID(),
        .WorldMatrix = node->World().RowMatrix(),
    });

    int count;
    auto child = node->GetChildren(&count);
    for (int i = 0; i < count; ++i, ++child)
    {
        TraverseNode(drawlist, *child);
    }
}

void TraverseMesh(DrawList *drawlist, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
{
    auto mesh = node->Mesh();
    if (mesh)
    {

        auto &submeshes = mesh->submeshes;
        for (int i = 0; i < (int)submeshes.size(); ++i)
        {
            if (filter(submeshes[i].material))
            {
                drawlist->Meshes.push_back({
                    .NodeID = node->ID(),
                    .Mesh = mesh,
                    .SubmeshIndex = i,
                });
            }
        }
    }

    int count;
    auto child = node->GetChildren(&count);
    for (int i = 0; i < count; ++i, ++child)
    {
        TraverseMesh(drawlist, *child, filter);
    }
}

static void UpdateDrawListIf(SceneView *view, const Scene *scene, const FilterFunc &filter)
{
    if (view->ShowGrid)
    {
        for (auto &node : scene->gizmoNodes)
        {
            TraverseMesh(&view->Drawlist, node, filter);
        }
    }
    if (view->ShowVR)
    {
        for (auto &node : scene->vrNodes)
        {
            TraverseMesh(&view->Drawlist, node, filter);
        }
    }
    for (auto &node : scene->sceneNodes)
    {
        TraverseMesh(&view->Drawlist, node, filter);
    }
}

void SceneView::UpdateDrawList(const Scene *scene)
{
    Drawlist.Clear();

    //
    // node
    //
    for (auto &node : scene->gizmoNodes)
    {
        TraverseNode(&Drawlist, node);
    }
    for (auto &node : scene->vrNodes)
    {
        TraverseNode(&Drawlist, node);
    }
    for (auto &node : scene->sceneNodes)
    {
        TraverseNode(&Drawlist, node);
    }

    //
    // mesh
    //
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
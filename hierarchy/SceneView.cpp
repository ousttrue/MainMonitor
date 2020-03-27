#include "SceneView.h"
#include "Scene.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"
#include "SceneMaterial.h"
#include "Shader.h"
#include <functional>

namespace hierarchy
{
using FilterFunc = std::function<bool(const SceneMaterialPtr &)>;

void TraverseMesh(DrawList *drawlist, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
{
    auto mesh = node->Mesh();
    if (mesh)
    {

        auto &submeshes = mesh->submeshes;
        for (int i = 0; i < (int)submeshes.size(); ++i)
        {
            auto &material = submeshes[i].material;
            if (filter(material))
            {
                auto shader = material->shader->Compiled();
                if (shader)
                {
                    auto m = node->World().RowMatrix();
                    CBValue values[] = {
                        {.semantic = ConstantSemantics::NODE_WORLD,
                         .p = &m,
                         .size = sizeof(m)}};
                    drawlist->PushCB(shader->VS.DrawCB(), values, _countof(values));
                    drawlist->Items.push_back({
                        .Mesh = mesh,
                        .SubmeshIndex = i,
                    });
                }
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
#include "DrawList.h"
#include "SceneNode.h"
#include "SceneMesh.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"

namespace hierarchy
{

void DrawList::Traverse(const std::shared_ptr<SceneNode> &node)
{

    auto mesh = node->Mesh();
    if (mesh)
    {
        Nodes.push_back({
            .NodeID = node->ID(),
            .WorldMatrix = node->World().RowMatrix(),
        });
        Meshes.push_back({
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
        Traverse(*child);
    }
}

} // namespace hierarchy

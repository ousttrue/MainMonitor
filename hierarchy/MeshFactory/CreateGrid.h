#pragma once
#include "VertexBuffer.h"
#include "ShaderManager.h"

namespace hierarchy
{
    
///
/// 無限グリッド
///
static std::shared_ptr<hierarchy::SceneMesh> CreateGrid()
{
    struct GridVertex
    {
        std::array<float, 2> position;
        std::array<float, 2> uv;
    };
    GridVertex vertices[] = {
        {{-1, 1}, {0, 0}},
        {{-1, -1}, {0, 1}},
        {{1, -1}, {1, 1}},
        {{1, 1}, {1, 0}},
    };
    uint16_t indices[] = {
        0, 1, 2, //
        2, 3, 0, //
    };
    auto mesh = hierarchy::SceneMesh::Create();
    mesh->vertices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Vertex,
        sizeof(vertices[0]), vertices, sizeof(vertices));
    mesh->indices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Index,
        2, indices, sizeof(indices));
    {
        auto material = hierarchy::SceneMaterial::Create();
        material->shader = hierarchy::ShaderManager::Instance().get("grid");
        mesh->submeshes.push_back({.drawCount = _countof(indices),
                                   .material = material});
    }
    return mesh;
}

} // namespace hierarchy

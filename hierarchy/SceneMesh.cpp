#include "SceneMesh.h"

namespace hierarchy
{

SceneMeshPtr SceneMesh::Create()
{
    return SceneMeshPtr(new SceneMesh);
}

std::shared_ptr<SceneMesh> SceneMesh::CreateDynamic(int vertexReserve, int indexReserve)
{
    auto mesh = SceneMeshPtr(new SceneMesh);

    // vertices
    {
        VertexBuffer buffer{
            .semantic = Semantics::PositionNormalColor,
            .valueType = ValueType::Float10,
            .isDynamic = true,
        };
        // auto bytes = (uint8_t *)p;
        buffer.buffer.resize(vertexReserve);
        mesh->SetVertices(buffer);
    }

    // indices
    {
        mesh->m_indices = VertexBuffer{
            .semantic = Semantics::Index,
            .valueType = ValueType::UInt16,
            .isDynamic = true,
        };
        mesh->m_indices.buffer.resize(indexReserve);
    }

    return mesh;
}

void SceneMesh::SetVertices(Semantics semantic, ValueType valueType, const void *p, uint32_t size)
{
    VertexBuffer buffer;
    buffer.semantic = semantic;
    buffer.valueType = valueType;
    auto bytes = (uint8_t *)p;
    buffer.buffer.assign(bytes, bytes + size);
    SetVertices(buffer);
}

const VertexBuffer *SceneMesh::GetVertices(Semantics semantic)
{
    for (auto &v : m_vertices)
    {
        if (v.semantic == semantic)
        {
            return &v;
        }
    }
    return nullptr;
}

void SceneMesh::SetIndices(ValueType valueType, const void *indices, uint32_t size)
{
    auto bytes = (uint8_t *)indices;
    m_indices.semantic = Semantics::Index;
    m_indices.valueType = valueType;
    m_indices.buffer.assign(bytes, bytes + size);
}

} // namespace hierarchy
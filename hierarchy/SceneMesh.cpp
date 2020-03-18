#include "SceneMesh.h"
#include "SceneNode.h"
#include <algorithm>

namespace hierarchy
{

void VertexBuffer::Append(const VertexBuffer &vb)
{
    if (semantic != vb.semantic)
    {
        throw;
    }
    if (stride != vb.stride)
    {
        throw;
    }
    buffer.reserve(buffer.size() + vb.buffer.size());
    std::copy(vb.buffer.begin(), vb.buffer.end(), std::back_inserter(buffer));
}

void SceneMeshSkin::Update()
{
    skinningMatrices.resize(inverseBindMatrices.size());
    for(size_t i=0; i<inverseBindMatrices.size(); ++i)
    {
        auto joint = joints[i];
        skinningMatrices[i] = falg::RowMatrixMul(inverseBindMatrices[i], joint->World().RowMatrix());
    }
}

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
            .semantic = Semantics::Interleaved,
            .stride = 40,
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
            .stride = 2,
            .isDynamic = true,
        };
        mesh->m_indices.buffer.resize(indexReserve);
    }

    return mesh;
}

void SceneMesh::AddSubmesh(const std::shared_ptr<SceneMesh> &mesh)
{
    if (!m_vertices.size() == mesh->m_vertices.size())
    {
        throw;
    }
    auto indexOffset = m_vertices.front().Count();
    for (auto &v : m_vertices)
    {
        if (v.Count() != indexOffset)
        {
            throw;
        }
    }

    for (auto &src : mesh->m_vertices)
    {
        bool found = false;
        for (auto &dst : m_vertices)
        {
            if (src.semantic == dst.semantic)
            {
                found = true;
                dst.Append(src);
                break;
            }
        }
        if (!found)
        {
            throw;
        }
    }

    auto last = m_indices.Count();
    m_indices.Append(mesh->m_indices);
    if (m_indices.stride == 2)
    {
        auto src = (uint16_t *)mesh->m_indices.buffer.data();
        auto dst = (uint16_t *)m_indices.buffer.data() + last;
        for (size_t i = 0; i < mesh->m_indices.Count(); ++i, ++src, ++dst)
        {
            *dst = *src + indexOffset;
        }
    }
    else if (m_indices.stride == 4)
    {
        auto src = (uint32_t *)mesh->m_indices.buffer.data();
        auto dst = (uint32_t *)m_indices.buffer.data() + last;
        for (size_t i = 0; i < mesh->m_indices.Count(); ++i, ++src, ++dst)
        {
            *dst = *src + indexOffset;
        }
    }
    else
    {
        throw;
    }

    if (mesh->submeshes.size() != 1)
    {
        throw;
    }
    submeshes.push_back(mesh->submeshes.front());
}

void SceneMesh::SetVertices(Semantics semantic, uint32_t stride, const void *p, uint32_t size)
{
    VertexBuffer buffer;
    buffer.semantic = semantic;
    buffer.stride = stride;
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

void SceneMesh::SetIndices(uint32_t stride, const void *indices, uint32_t size)
{
    auto bytes = (uint8_t *)indices;
    m_indices.semantic = Semantics::Index;
    m_indices.stride = stride;
    m_indices.buffer.assign(bytes, bytes + size);
}

} // namespace hierarchy
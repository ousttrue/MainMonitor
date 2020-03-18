#include "SceneMesh.h"
#include "SceneNode.h"
#include <algorithm>

namespace hierarchy
{

void VertexBuffer::Append(const std::shared_ptr<VertexBuffer> &vb)
{
    if (semantic != vb->semantic)
    {
        throw;
    }
    if (stride != vb->stride)
    {
        throw;
    }
    buffer.reserve(buffer.size() + vb->buffer.size());
    std::copy(vb->buffer.begin(), vb->buffer.end(), std::back_inserter(buffer));
}

void SceneMeshSkin::Update(const void *vertices, uint8_t stride, uint8_t vertexCount)
{
    // update skining Matrices
    skinningMatrices.resize(inverseBindMatrices.size());
    for (size_t i = 0; i < inverseBindMatrices.size(); ++i)
    {
        auto joint = joints[i];
        skinningMatrices[i] = falg::RowMatrixMul(inverseBindMatrices[i], joint->World().RowMatrix());
    }

    // create new vertexbuffer
    auto src = (const uint8_t *)vertices;
    cpuSkiningBuffer.resize(stride * vertexCount);
    auto dst = cpuSkiningBuffer.data();
    for (uint8_t i = 0; i < vertexCount; ++i, src += stride, dst += stride)
    {
        // auto value = falg::RowMatrixApplyPosition(skinningMatrices[i], *(std::array<float, 3> *)src);
        // *(std::array<float, 3> *)dst = value;
    }
}

SceneMeshPtr SceneMesh::Create()
{
    return SceneMeshPtr(new SceneMesh);
}

std::shared_ptr<SceneMesh> SceneMesh::CreateDynamic(
    uint32_t vertexReserve, uint32_t vertexStride,
    uint32_t indexReserve, uint32_t indexStride)
{
    auto mesh = SceneMeshPtr(new SceneMesh);

    mesh->vertices = VertexBuffer::CreateDynamic(
        Semantics::Vertex,
        vertexStride,
        vertexReserve);

    mesh->indices = VertexBuffer::CreateDynamic(
        Semantics::Index,
        indexStride,
        indexReserve);

    return mesh;
}

void SceneMesh::AddSubmesh(const std::shared_ptr<SceneMesh> &mesh)
{
    auto indexOffset = vertices->Count();

    auto src = mesh->vertices;
    {
        bool found = false;
        auto dst = vertices;
        {
            if (src->semantic == dst->semantic)
            {
                found = true;
                dst->Append(src);
            }
            else
            {
                throw;
            }
        }
    }

    auto last = indices->Count();
    indices->Append(mesh->indices);
    if (indices->stride == 2)
    {
        auto src = (uint16_t *)mesh->indices->buffer.data();
        auto dst = (uint16_t *)indices->buffer.data() + last;
        auto count = mesh->indices->Count();
        for (size_t i = 0; i < count; ++i, ++src, ++dst)
        {
            *dst = *src + indexOffset;
        }
    }
    else if (indices->stride == 4)
    {
        auto src = (uint32_t *)mesh->indices->buffer.data();
        auto dst = (uint32_t *)indices->buffer.data() + last;
        auto count = mesh->indices->Count();
        for (size_t i = 0; i < count; ++i, ++src, ++dst)
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

} // namespace hierarchy
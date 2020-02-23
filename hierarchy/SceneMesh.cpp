#include "SceneMesh.h"

namespace hierarchy
{
std::shared_ptr<SceneMesh> SceneMesh::Create()
{
    static int s_id = 0;
    auto model = std::shared_ptr<SceneMesh>(new SceneMesh(s_id++));
    return model;
}

void SceneMesh::SetVertices(const uint8_t *p, int byteLength, int stride)
{
    m_vertices.assign(p, p + byteLength);
    m_vertexStride = stride;
}

void SceneMesh::SetIndices(const uint8_t *p, int byteLength, int stride)
{
    m_indices.assign(p, p + byteLength);
    m_indexStride = stride;
}
} // namespace scngraph
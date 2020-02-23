#include "SceneMesh.h"

namespace hierarchy
{

SceneMeshPtr SceneMesh::Create()
{
    return SceneMeshPtr(new SceneMesh);
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
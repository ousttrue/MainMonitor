#include "Model.h"

std::shared_ptr<Model> Model::Create()
{
    static int s_id = 0;
    auto model = std::shared_ptr<Model>(new Model(s_id++));
    return model;
}

void Model::SetVertices(const uint8_t *p, int byteLength, int stride)
{
    m_vertices.assign(p, p + byteLength);
    m_vertexStride = stride;
}

void Model::SetIndices(const uint8_t *p, int byteLength, int stride)
{
    m_indices.assign(p, p + byteLength);
    m_indexStride = stride;
}

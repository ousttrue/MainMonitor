#include "Model.h"

std::shared_ptr<Model> Model::Create()
{
    auto model = std::make_shared<Model>();
    return model;
}

void Model::SetVeritces(const uint8_t *p, int byteLength, int stride)
{
    m_vertices.assign(p, p + byteLength);
    m_vertexStride = stride;
}

void Model::SetIndices(const uint8_t *p, int byteLength, int stride)
{
    m_indices.assign(p, p + byteLength);
    m_indexStride = stride;
}

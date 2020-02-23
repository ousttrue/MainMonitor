#include "SceneModel.h"

namespace hierarchy
{
std::shared_ptr<SceneModel> SceneModel::Create()
{
    static int s_id = 0;
    auto model = std::shared_ptr<SceneModel>(new SceneModel(s_id++));
    return model;
}

void SceneModel::SetVertices(const uint8_t *p, int byteLength, int stride)
{
    m_vertices.assign(p, p + byteLength);
    m_vertexStride = stride;
}

void SceneModel::SetIndices(const uint8_t *p, int byteLength, int stride)
{
    m_indices.assign(p, p + byteLength);
    m_indexStride = stride;
}
} // namespace scngraph
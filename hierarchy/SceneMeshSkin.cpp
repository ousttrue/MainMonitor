#include "SceneMeshSkin.h"
#include "SceneNode.h"
#include <falg.h>

namespace hierarchy
{

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
        auto value = falg::RowMatrixApplyPosition(skinningMatrices[i], *(std::array<float, 3> *)src);
        *(std::array<float, 3> *)dst = value;
    }
}

} // namespace hierarchy

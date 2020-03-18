#include "SceneMeshSkin.h"
#include "SceneNode.h"
#include <falg.h>

namespace hierarchy
{

void SceneMeshSkin::Update(const void *vertices, uint32_t stride, uint32_t vertexCount)
{
    // update skining Matrices
    skiningMatrices.resize(inverseBindMatrices.size());
    for (size_t i = 0; i < inverseBindMatrices.size(); ++i)
    {
        auto joint = joints[i];
        skiningMatrices[i] = falg::RowMatrixMul(inverseBindMatrices[i], joint->World().RowMatrix());
    }

    // create new vertexbuffer
    auto src = (const uint8_t *)vertices;
    cpuSkiningBuffer.resize(stride * vertexCount);
    uint8_t *dst = cpuSkiningBuffer.data();
    for (uint32_t i = 0; i < vertexCount; ++i, src += stride, dst += stride)
    {
        auto skining = vertexSkiningArray[i];
        auto position = *(std::array<float, 3> *)src;
        std::array<float, 3> value{};
        if (skining.weights[0] > 0)
        {
            auto joint = joints[skining.joints[0]];
            value += falg::RowMatrixApplyPosition(skiningMatrices[skining.joints[0]], position) * skining.weights[0];
        }
        if (skining.weights[1] > 0)
        {
            auto joint = joints[skining.joints[1]];
            value += falg::RowMatrixApplyPosition(skiningMatrices[skining.joints[1]], position) * skining.weights[1];
        }
        if (skining.weights[2] > 0)
        {
            auto joint = joints[skining.joints[2]];
            value += falg::RowMatrixApplyPosition(skiningMatrices[skining.joints[2]], position) * skining.weights[2];
        }
        if (skining.weights[3] > 0)
        {
            auto joint = joints[skining.joints[3]];
            value += falg::RowMatrixApplyPosition(skiningMatrices[skining.joints[3]], position) * skining.weights[3];
        }
        // auto value = falg::RowMatrixApplyPosition(skiningMatrices[i], *(std::array<float, 3> *)src);
        memcpy(dst, src, stride);
        *(std::array<float, 3> *)dst = value;
    }
}

} // namespace hierarchy

#pragma once
#include <vector>
#include <array>
#include <memory>
#include <stdint.h>

struct VertexSkining
{
    std::array<uint16_t, 4> joints;
    std::array<float, 4> weights;
};

namespace hierarchy
{
class SceneNode;
class SceneMeshSkin
{
public:
    // skining information
    std::shared_ptr<SceneNode> root;
    std::vector<std::shared_ptr<SceneNode>> joints;
    std::vector<std::array<float, 16>> inverseBindMatrices;
    std::vector<VertexSkining> vertexSkiningArray;

    // runtime buffer
    std::vector<std::array<float, 16>> skiningMatrices;
    std::vector<uint8_t> cpuSkiningBuffer;

    void Update(const void *vertices, uint32_t stride, uint32_t vertexCount);
};
using SceneMeshSkinPtr = std::shared_ptr<SceneMeshSkin>;

} // namespace hierarchy

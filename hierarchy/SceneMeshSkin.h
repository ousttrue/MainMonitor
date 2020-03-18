#pragma once
#include <vector>
#include <array>
#include <memory>
#include <stdint.h>

namespace hierarchy
{
class SceneNode;
class SceneMeshSkin
{
public:
    std::vector<std::shared_ptr<SceneNode>> joints;
    std::vector<std::array<float, 16>> inverseBindMatrices;
    std::vector<std::array<float, 16>> skinningMatrices;
    std::vector<uint8_t> cpuSkiningBuffer;
    void Update(const void *vertices, uint8_t stride, uint8_t vertexCount);
};
using SceneMeshSkinPtr = std::shared_ptr<SceneMeshSkin>;

} // namespace hierarchy

#pragma once
#include <array>
#include <vector>
#include <memory>

namespace hierarchy
{

class SceneMesh;
class SceneNode;

struct DrawList
{
    struct Buffer
    {
        uint8_t *Ptr;
        uint32_t Bytes;
        uint32_t Stride;
    };
    struct DrawItem
    {
        // node
        std::array<float, 16> WorldMatrix;
        // mesh
        std::shared_ptr<SceneMesh> Mesh;
        Buffer Vertices{};
        Buffer Indices{};
        int SubmeshIndex;
    };
    std::vector<DrawItem> Items;

    void Clear()
    {
        Items.clear();
    }
    // void Traverse(const std::shared_ptr<SceneNode> &node);
};

} // namespace hierarchy

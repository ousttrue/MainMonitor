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
    struct NodeInfo
    {
        int NodeID;
        std::array<float, 16> WorldMatrix;
    };
    std::vector<NodeInfo> Nodes;

    struct Buffer
    {
        uint8_t *Ptr;
        uint32_t Bytes;
        uint32_t Stride;
    };
    struct MeshInfo
    {
        int NodeID;
        std::shared_ptr<SceneMesh> Mesh;
        Buffer Vertices{};
        Buffer Indices{};
    };
    std::vector<MeshInfo> Meshes;

    void Clear()
    {
        Nodes.clear();
        Meshes.clear();
    }
    void Traverse(const std::shared_ptr<SceneNode> &node);
};

} // namespace hierarchy

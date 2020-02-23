#pragma once
#include "SceneMesh.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>

namespace hierarchy
{

//
// * worldMatrix
// * meshes
//
class SceneNode
{
    // unique
    int m_id = -1;
    std::vector<std::shared_ptr<SceneMesh>> m_meshes;

    SceneNode(int id)
        : m_id(id)
    {
    }

public:
    static std::shared_ptr<SceneNode> Create();

    int ID() const { return m_id; }

    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 world{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    };
    ModelConstantBuffer Data;

    void AddMesh(const std::shared_ptr<SceneMesh> &mesh)
    {
        m_meshes.push_back(mesh);
    }
    const std::shared_ptr<SceneMesh> *GetMeshes(int *pCount) const
    {
        *pCount = (int)m_meshes.size();
        return m_meshes.data();
    }
};
using SceneNodePtr = std::shared_ptr<SceneNode>;

} // namespace hierarchy

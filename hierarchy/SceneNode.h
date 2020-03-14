#pragma once
#include "SceneMesh.h"
// #include <DirectXMath.h>
#include <vector>
#include <memory>
#include <fpalg.h>

namespace hierarchy
{

//
// * worldMatrix
// * meshes
//
class SceneNode : public std::enable_shared_from_this<SceneNode>
{
    // unique
    int m_id = -1;
    std::string m_name;
    std::vector<std::shared_ptr<SceneMesh>> m_meshes;
    bool m_enableGizmo = false;

    std::vector<std::shared_ptr<SceneNode>> m_children;
    std::weak_ptr<SceneNode> m_parent;

    SceneNode(int id)
        : m_id(id)
    {
    }

public:
    fpalg::Transform Transform{};

    static std::shared_ptr<SceneNode> Create(const std::string &name);

    int ID() const { return m_id; }

    bool EnableGizmo() const { return m_enableGizmo; }
    void EnableGizmo(bool enable) { m_enableGizmo = enable; }

    void AddMesh(const std::shared_ptr<SceneMesh> &mesh)
    {
        m_meshes.push_back(mesh);
    }
    const std::shared_ptr<SceneMesh> *GetMeshes(int *pCount) const
    {
        *pCount = (int)m_meshes.size();
        return m_meshes.data();
    }

    void AddChild(const std::shared_ptr<SceneNode> &child)
    {
        auto self = shared_from_this();
        child->m_parent = self;
        m_children.push_back(child);
    }
    std::shared_ptr<SceneNode> Parent()
    {
        return m_parent.lock();
    }
    const std::shared_ptr<SceneNode> *GetChildren(int *pCount) const
    {
        *pCount = (int)m_children.size();
        return m_children.data();
    }
};
using SceneNodePtr = std::shared_ptr<SceneNode>;

} // namespace hierarchy

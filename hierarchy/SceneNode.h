#pragma once
#include "SceneMesh.h"
// #include <DirectXMath.h>
#include <vector>
#include <memory>
#include <falg.h>

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
    std::shared_ptr<SceneMesh> m_mesh;
    bool m_enableGizmo = false;

    std::vector<std::shared_ptr<SceneNode>> m_children;
    std::weak_ptr<SceneNode> m_parent;

    falg::Transform m_local{};
    falg::Transform m_world{};

    SceneNode(int id)
        : m_id(id)
    {
    }

public:
    const SceneMeshPtr &Mesh() const { return m_mesh; }
    void Mesh(const SceneMeshPtr &mesh) { m_mesh = mesh; }
    falg::Transform &World() { return m_world; }
    void World(const falg::Transform &world, bool updateChildren = true)
    {
        auto parent = Parent();
        if (parent)
        {
            m_local = world * parent->World().Inverse();
        }
        else
        {
            m_local = world;
        }

        if (updateChildren)
        {
            for (auto &child : m_children)
            {
                child->UpdateWorld(World());
            }
        }
    }

    falg::Transform &Local() { return m_local; }
    void Local(const falg::Transform &local, bool updateChildren = true)
    {
        m_local = local;
        auto parent = Parent();

        if (parent)
        {
            m_world = local * parent->World();
        }
        else
        {
            m_world = local;
        }

        if (updateChildren)
        {
            for (auto &child : m_children)
            {
                child->UpdateWorld(World());
            }
        }
    }

    void UpdateWorld()
    {
        auto parent = Parent();
        if (parent)
        {
            UpdateWorld(parent->World());
        }
        else
        {
            UpdateWorld(falg::Transform{});
        }
    }

    void UpdateWorld(const falg::Transform &parent)
    {
        m_world = Local() * parent;

        for (auto &child : m_children)
        {
            child->UpdateWorld(m_world);
        }
    }

    static std::shared_ptr<SceneNode> Create(const std::string &name);

    int ID() const { return m_id; }
    std::string Name() const { return m_name; }
    void Name(const std::string &name) { m_name = name; }

    bool EnableGizmo() const { return m_enableGizmo; }
    void EnableGizmo(bool enable) { m_enableGizmo = enable; }

    void AddChild(const std::shared_ptr<SceneNode> &child)
    {
        auto self = shared_from_this();
        child->m_parent = self;
        m_children.push_back(child);
    }
    std::shared_ptr<SceneNode> Parent() const
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

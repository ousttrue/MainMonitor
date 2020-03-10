#pragma once
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include "SceneNode.h"
#include "SceneMaterial.h"
#include "SceneMesh.h"

namespace hierarchy
{
class Scene
{
    std::vector<SceneNodePtr> m_nodes;

public:
    Scene();
    const SceneNodePtr *GetNodes(int *pCount) const
    {
        *pCount = (int)m_nodes.size();
        return m_nodes.data();
    }
    const SceneNodePtr &GetNode(int index) const
    {
        return m_nodes[index];
    }
    SceneNodePtr GetOrCreateNode(int index)
    {
        auto node = m_nodes[index];
        if(!node)
        {
            node = SceneNode::Create();
            m_nodes[index] = node;
        }
        return node;
    }
    int Count() const { return (int)m_nodes.size(); }

    void AddNullNode()
    {
        m_nodes.push_back(nullptr);
    }
    void AddMeshNode(const std::shared_ptr<hierarchy::SceneMesh> &mesh)
    {
        auto node = SceneNode::Create();
        node->AddMesh(mesh);
        m_nodes.push_back(node);
    }
    void LoadFromPath(const std::string &path);
    void LoadFromPath(const std::wstring &path);
    void LoadGlbBytes(const uint8_t *p, int size);
};
} // namespace hierarchy
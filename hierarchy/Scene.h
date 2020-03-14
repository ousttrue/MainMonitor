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
    std::vector<SceneNodePtr> m_rootNodes;

public:
    Scene();
    const SceneNodePtr *GetRootNodes(int *pCount) const
    {
        *pCount = (int)m_rootNodes.size();
        return m_rootNodes.data();
    }
    const SceneNodePtr &GetRootNode(int index) const
    {
        return m_rootNodes[index];
    }
    int RootNodeCount() const { return (int)m_rootNodes.size(); }
    void AddRootNode(const SceneNodePtr &node)
    {
        m_rootNodes.push_back(node);
    }
};

} // namespace hierarchy

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

public:
    std::vector<SceneNodePtr> gizmoNodes;
    std::vector<SceneNodePtr> vrNodes;
    std::vector<SceneNodePtr> sceneNodes;

    // single selection
    std::weak_ptr<hierarchy::SceneNode> selected;

    Scene();
};

} // namespace hierarchy

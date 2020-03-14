#include "SceneNode.h"

namespace hierarchy
{

SceneNodePtr SceneNode::Create(const std::string &name)
{
    static int s_id = 0;
    auto node = SceneNodePtr(new SceneNode(s_id++));
    node->m_name = name;
    return node;
}

} // namespace hierarchy
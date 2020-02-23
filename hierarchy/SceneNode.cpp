#include "SceneNode.h"

namespace hierarchy
{

SceneNodePtr SceneNode::Create()
{
    static int s_id = 0;
    auto node = SceneNodePtr(new SceneNode(s_id++));
    return node;
}

} // namespace hierarchy
#pragma once
#include "SceneNode.h"

namespace hierarchy
{

struct SceneGltf
{
    static SceneNodePtr LoadFromPath(const std::string &path);
    static SceneNodePtr LoadFromPath(const std::wstring &path);
    static SceneNodePtr LoadGlbBytes(const uint8_t *p, int size);
};

} // namespace hierarchy

#pragma once
#include "SceneNode.h"

namespace hierarchy
{

struct SceneGltf
{
    static SceneNodePtr LoadFromPath(const std::filesystem::path &path);
    static SceneNodePtr LoadGlbBytes(const uint8_t *p, int size);
};

} // namespace hierarchy

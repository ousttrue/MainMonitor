#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include "SceneImage.h"
#include "ShaderWatcher.h"

namespace hierarchy
{

enum class AlphaMode
{
    Opaque,
    Mask,
    Blend,
};

class SceneMaterial
{

public:
    static std::shared_ptr<SceneMaterial> Create();

    std::string name;
    ShaderWatcherPtr shader;
    AlphaMode alphaMode{};
    float alphaCutoff = 0;
    SceneImagePtr colorImage;
};
using SceneMaterialPtr = std::shared_ptr<SceneMaterial>;

} // namespace hierarchy

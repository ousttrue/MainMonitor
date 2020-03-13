#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include "SceneImage.h"
#include "ShaderWatcher.h"

namespace hierarchy
{

class SceneMaterial
{

public:
    static std::shared_ptr<SceneMaterial> Create();

    std::string name;
    ShaderWatcherPtr shader;
    SceneImagePtr colorImage;
};
using SceneMaterialPtr = std::shared_ptr<SceneMaterial>;

} // namespace hierarchy

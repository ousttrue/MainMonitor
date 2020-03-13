#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include "SceneImage.h"

namespace hierarchy
{

class SceneMaterial
{

public:
    static std::shared_ptr<SceneMaterial> Create();

    SceneImagePtr colorImage;
    std::string name;
    std::string shaderName;
};
using SceneMaterialPtr = std::shared_ptr<SceneMaterial>;

} // namespace hierarchy

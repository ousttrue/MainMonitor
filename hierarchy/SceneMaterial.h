#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include "SceneImage.h"

namespace hierarchy
{

class SceneMaterial
{
    SceneImagePtr m_colorImage;
    std::string m_name;

public:
    static std::shared_ptr<SceneMaterial> Create();
    void SetName(const std::string &name) { m_name = name; }
    void SetImage(const SceneImagePtr &image) { m_colorImage = image; }
};
using SceneMaterialPtr = std::shared_ptr<SceneMaterial>;

} // namespace hierarchy

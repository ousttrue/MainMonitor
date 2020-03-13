#include "SceneMaterial.h"

namespace hierarchy
{

std::shared_ptr<SceneMaterial> SceneMaterial::Create()
{
    auto material = SceneMaterialPtr(new SceneMaterial);
    material->shaderName = "default";
    return material;
}

} // namespace hierarchy
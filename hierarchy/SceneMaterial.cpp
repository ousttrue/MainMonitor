#include "SceneMaterial.h"
#include "ShaderManager.h"

namespace hierarchy
{

std::shared_ptr<SceneMaterial> SceneMaterial::Create()
{
    auto material = SceneMaterialPtr(new SceneMaterial);
    material->shader = ShaderManager::Instance().getDefault();
    return material;
}

} // namespace hierarchy
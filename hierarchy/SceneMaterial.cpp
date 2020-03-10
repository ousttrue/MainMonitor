#include "SceneMaterial.h"

namespace hierarchy
{

std::shared_ptr<SceneMaterial> SceneMaterial::Create()
{
    return SceneMaterialPtr(new SceneMaterial);
}

} // namespace hierarchy
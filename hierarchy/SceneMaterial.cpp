#include "SceneMaterial.h"

std::string g_shaderSource =
#include "OpenVRRenderModel.hlsl"
    ;

namespace hierarchy
{

std::shared_ptr<SceneMaterial> SceneMaterial::Create()
{
    auto material =  SceneMaterialPtr(new SceneMaterial);
    material->shader = g_shaderSource;
    return material;
}

} // namespace hierarchy
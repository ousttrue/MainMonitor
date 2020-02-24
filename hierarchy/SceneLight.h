#pragma once
#include <DirectXMath.h>

namespace hierarchy
{
class SceneLight
{
public:
    DirectX::XMFLOAT3 LightDirection = {0, -1, 0};
    DirectX::XMFLOAT3 LightColor = {1, 1, 1};
};
} // namespace hierarchy

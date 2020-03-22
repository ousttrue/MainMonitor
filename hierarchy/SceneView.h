#pragma once
#include <memory>
#include <array>

namespace hierarchy
{

struct SceneView
{
    int Width = 0;
    int Height = 0;
    std::array<float, 16> Projection = {};
    std::array<float, 16> View = {};
    std::array<float, 3> CameraPosition = {0, 0, 0};
    float CameraFovYRadians = 1.0f;
};
using SceneViewPtr = std::shared_ptr<SceneView>;

} // namespace hierarchy

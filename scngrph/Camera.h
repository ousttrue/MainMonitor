#pragma once
#include <DirectXMath.h>

namespace screenstate
{
struct ScreenState;
}

namespace scngrph
{
/// TurnTable
class Camera
{
    // view
    float m_yaw = 0;
    float m_pitch = 0;
    DirectX::XMFLOAT3 m_translation = {0, 0, -7};

    // projection
    float m_near = 0.1f;
    float m_far = 100.0f;
    float m_fovY = 30.0f / 180.0f * DirectX::XM_PI;
    float m_aspectRatio = 1.0f;

    void Calc();

public:
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };
    SceneConstantBuffer Data;
    Camera();
    bool OnFrame(const screenstate::ScreenState &state, const screenstate::ScreenState &prev);
};
} // namespace scngrph
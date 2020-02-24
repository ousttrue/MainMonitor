#include "SceneCamera.h"
#include "ScreenState.h"

using namespace DirectX;

namespace hierarchy
{
SceneCamera::SceneCamera()
{
    Calc();
}

void SceneCamera::Calc()
{
    {
        auto m = XMMatrixPerspectiveFovRH(m_fovY, m_aspectRatio, m_near, m_far);
        XMStoreFloat4x4(&projection, m);
    }

    {
        auto yaw = XMMatrixRotationY(m_yaw);
        auto pitch = XMMatrixRotationX(m_pitch);
        auto t = XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
        auto m = yaw * pitch * t;
        XMStoreFloat4x4(&view, m);
    }
}

bool SceneCamera::OnFrame(const screenstate::ScreenState &state, const screenstate::ScreenState &prev)
{
    bool changed = false;
    auto dx = state.X - prev.X;
    auto dy = state.Y - prev.Y;
    auto dt = state.DeltaSeconds(prev);
    if (state.Has(screenstate::MouseButtonFlags::RightDown) && prev.Has(screenstate::MouseButtonFlags::RightDown))
    {
        // right drag
        changed = true;
        auto f = 1.0f * dt;
        m_yaw += dx * f;
        m_pitch += dy * f;
    }
    if (state.Has(screenstate::MouseButtonFlags::MiddleDown) && prev.Has(screenstate::MouseButtonFlags::MiddleDown))
    {
        // Middle drag
        changed = true;
        const auto f = m_translation.z * (float)tan(m_fovY / 2) / state.Height * 2;

        m_translation.x -= dx * f;
        m_translation.y += dy * f;
    }
    if (state.Has(screenstate::MouseButtonFlags::WheelMinus))
    {
        changed = true;
        m_translation.z *= 1.1f;
    }
    if (state.Has(screenstate::MouseButtonFlags::WheelPlus))
    {
        changed = true;
        m_translation.z *= 0.9f;
    }
    if (!state.SameSize(prev))
    {
        changed = true;
        m_aspectRatio = state.AspectRatio();
    }

    if (!changed)
    {
        return false;
    }

    Calc();
    return true;
}
} // namespace hierarchy
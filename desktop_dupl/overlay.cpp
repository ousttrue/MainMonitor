#include "overlay.h"
#include <stdint.h>
#include <openvr.h>

const float DESKTOP_HORIZONTAL_METER = 3.6f;

class OverlayImpl
{
    vr::VROverlayHandle_t g_ulOverlayHandle = vr::k_ulOverlayHandleInvalid;
    vr::VROverlayHandle_t g_ulOverlayThumbnailHandle = vr::k_ulOverlayHandleInvalid;

public:
    OverlayImpl()
    {
    }

    ~OverlayImpl()
    {
        vr::VROverlay()->ClearOverlayTexture(g_ulOverlayHandle);
        vr::VROverlay()->DestroyOverlay(g_ulOverlayHandle);
    }

    bool InitOpenVR(const char *key, const char *name)
    {
        vr::EVRInitError initError = vr::VRInitError_None;
        vr::VR_Init(&initError, vr::VRApplication_Overlay);
        if (initError != vr::VRInitError_None)
        {
            return false;
        }

        vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay(
            key, name,
            &g_ulOverlayHandle, &g_ulOverlayThumbnailHandle);
        if (overlayError != vr::VROverlayError_None)
        {
            return false;
        }

        vr::VROverlay()->SetOverlayAlpha(g_ulOverlayHandle, 1.0f);
        vr::VROverlay()->SetOverlayColor(g_ulOverlayHandle, 1.0f, 1.0f, 1.0f);
        vr::VROverlay()->SetOverlayWidthInMeters(g_ulOverlayHandle, DESKTOP_HORIZONTAL_METER);

        return true;
    }

    void SetSharedHandle(void *handle)
    {
        vr::Texture_t texture = {handle, vr::TextureType_DXGISharedHandle, vr::ColorSpace_Auto};
        vr::VROverlay()->SetOverlayTexture(g_ulOverlayHandle, &texture);
    }

    bool Loop(bool *isActive)
    {
        *isActive = false;
        if (!vr::VROverlay())
            return false;

        if (vr::VROverlay()->IsOverlayVisible(g_ulOverlayHandle))
        {
            *isActive = true;

            vr::VREvent_t Event;
            while (vr::VROverlay()->PollNextOverlayEvent(g_ulOverlayHandle, &Event, sizeof(Event)))
            {
                switch (Event.eventType)
                {
                case vr::VREvent_MouseMove:
                    break;
                case vr::VREvent_MouseButtonDown:
                    break;
                }
            }
        }
        return true;
    }
};

Overlay::Overlay()
    : m_impl(new OverlayImpl)
{
}

Overlay::~Overlay()
{
    delete m_impl;
}

bool Overlay::Initialize(const char *key, const char *name)
{
    return m_impl->InitOpenVR(key, name);
}

bool Overlay::Loop(bool *isActive)
{
    return m_impl->Loop(isActive);
}

void Overlay::SetSharedHanle(void *handle)
{
    m_impl->SetSharedHandle(handle);
}

#include "Window.h"
#include "Renderer.h"
#include <openvr.h>
#include <iostream>

class OpenVR
{
    vr::IVRSystem *m_system = nullptr;

public:
    ~OpenVR()
    {
    }

    bool Connect()
    {
        vr::EVRInitError initError = vr::VRInitError_None;
        m_system = vr::VR_Init(&initError, vr::VRApplication_Overlay);
        if (initError != vr::VRInitError_None)
        {
            return false;
        }

        return true;
    }

    void OnFrame()
    {
        // Process SteamVR events
        vr::VREvent_t event;
        while (m_system->PollNextEvent(&event, sizeof(event)))
        {
            // ProcessVREvent(event, pCommandList);
            switch (event.eventType)
            {
            case vr::VREvent_TrackedDeviceActivated:
            {
                std::cout << "VREvent_TrackedDeviceActivated: " << event.trackedDeviceIndex << std::endl;
            }
            break;
            case vr::VREvent_TrackedDeviceDeactivated:
            {
                std::cout << "VREvent_TrackedDeviceDeactivated: " << event.trackedDeviceIndex << std::endl;
            }
            break;
            case vr::VREvent_TrackedDeviceUpdated:
            {
                std::cout << "VREvent_TrackedDeviceUpdated: " << event.trackedDeviceIndex << std::endl;
            }
            break;
            }
        }
    }
};

int main()
{
    Window window;

    auto hwnd = window.Create(L"ExternalViewerClass", L"ExternalViewer");
    if (!hwnd)
    {
        return 1;
    }

    window.Show();

    OpenVR vr;
    if (!vr.Connect())
    {
        // return 2;
    }

    {
        Renderer renderer;
        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame();
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

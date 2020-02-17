#include "VR.h"
// #include <openvr.h>
// #include <string>
// #include <vector>
// #include <DirectXMath.h>
// #include <iostream>
// #include <functional>
// #include <list>
// #include <unordered_map>

static std::string GetTrackedDeviceString(vr::IVRSystem *system, vr::TrackedDeviceIndex_t index,
                                          vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *pError = NULL)
{
    auto byteLength = system->GetStringTrackedDeviceProperty(index, prop, NULL, 0, pError);
    if (byteLength == 0)
    {
        return "";
    }
    std::vector<char> buffer(byteLength);
    system->GetStringTrackedDeviceProperty(index, prop, buffer.data(), (uint32_t)buffer.size(), pError);

    auto end = buffer.begin();
    for (; end != buffer.end(); ++end)
    {
        if (*end == '\0')
        {
            break;
        }
    }
    return std::string(buffer.begin(), end);
}

static DirectX::XMFLOAT4X4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
    DirectX::XMFLOAT4X4 matrixObj(
        matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
        matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
        matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
        matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f);
    return matrixObj;
}

void VR::SetCallback(const OnModelLoad &callback)
{
    m_callback = callback;
}

bool VR::Connect()
{
    vr::EVRInitError initError = vr::VRInitError_None;
    m_system = vr::VR_Init(&initError, vr::VRApplication_Overlay);
    if (initError != vr::VRInitError_None)
    {
        return false;
    }

    for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
    {
        auto deviceClass = m_system->GetTrackedDeviceClass(i);
        if (deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid)
        {
            continue;
        }
        // std::cout << "tracker[" << i << "] ";
        auto driver = m_system->GetUint64TrackedDeviceProperty(i, vr::Prop_ParentDriver_Uint64);
        // std::cout << std::endl;

        StartLoadModel(i);
    }

    return true;
}

void VR::OnFrame()
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
            // std::cout << "VREvent_TrackedDeviceActivated: " <<  << std::endl;
            StartLoadModel(event.trackedDeviceIndex);
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

    m_system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0,
                                              m_poses, vr::k_unMaxTrackedDeviceCount);

    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
    {
        if (m_poses[i].bPoseIsValid)
        {
            auto found = m_trackers.find(i);
            if (found != m_trackers.end())
            {
                auto pose = ConvertSteamVRMatrixToMatrix4(m_poses[i].mDeviceToAbsoluteTracking);
                found->second->Matrix(pose);
            }
        }
    }
}

void VR::StartLoadModel(int index)
{
    if (!m_system->IsTrackedDeviceConnected(index))
    {
        return;
    }

    // try to find a model we've already set up
    vr::TrackedPropertyError tError;
    auto modelName = GetTrackedDeviceString(m_system, index, vr::Prop_RenderModelName_String, &tError);
    if (tError != vr::TrackedPropertyError::TrackedProp_Success)
    {
        return;
    }
    std::cout << "#" << index << " loadModel: " << modelName << std::endl;
    m_trackers.insert(std::make_pair(index, std::make_unique<Tracker>(index, modelName)));
}

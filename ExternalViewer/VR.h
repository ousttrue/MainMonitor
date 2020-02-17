#pragma once
#include <openvr.h>
#include <list>
#include <memory>

class VR
{
    vr::IVRSystem *m_system = nullptr;
    vr::TrackedDevicePose_t m_poses[vr::k_unMaxTrackedDeviceCount] = {};
    std::list<std::shared_ptr<struct LoadTask>> m_tasks;

public:
    std::shared_ptr<class Model> m_trackers[vr::k_unMaxTrackedDeviceCount] = {};
    bool Connect();
    void OnFrame();
    void StartLoadModel(int index);
};

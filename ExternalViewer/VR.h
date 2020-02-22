#pragma once
#include <openvr.h>
#include <list>
#include <memory>

namespace hierarchy
{
    class Scene;
}

class VR
{
    vr::IVRSystem *m_system = nullptr;
    vr::TrackedDevicePose_t m_poses[vr::k_unMaxTrackedDeviceCount] = {};
    std::list<std::shared_ptr<struct LoadTask>> m_tasks;

public:
    bool Connect();
    void OnFrame(hierarchy::Scene *scene);
    void StartLoadModel(int index);
};

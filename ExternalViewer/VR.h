#pragma once
#include <openvr.h>
#include <list>
#include <memory>
#include <unordered_map>

namespace hierarchy
{

class Scene;
class SceneNode;

} // namespace hierarchy

class VR
{
    vr::IVRSystem *m_system = nullptr;
    vr::TrackedDevicePose_t m_poses[vr::k_unMaxTrackedDeviceCount] = {};
    std::list<std::shared_ptr<struct LoadTask>> m_tasks;
    std::unordered_map<int, std::shared_ptr<hierarchy::SceneNode>> m_trackerNodeMap;

public:
    bool Connect();
    void OnFrame(hierarchy::Scene *scene);
    void StartLoadModel(int index);
};

#include "Scene.h"
#include "Model.h"

Scene::Scene(int trackerCount)
    : m_trackers(trackerCount)
{
}

void Scene::SetModel(int trackerID, const Item &model)
{
    m_trackers[trackerID] = model;
}

void Scene::SetPose(int trackerID, const DirectX::XMFLOAT4X4 &pose)
{
    if (trackerID < 0 || trackerID >= m_trackers.size())
    {
        return;
    }
    auto tracker = m_trackers[trackerID];
    if (tracker)
    {
        tracker->Data.world = pose;
    }
}

#pragma once
#include <memory>
#include <vector>
#include <d12util.h>

class Scene
{
    using Item = std::shared_ptr<scngrph::Model>;
    std::vector<Item> m_trackers;

public:
    Scene(int trackerCount);
    const Item *Data() const { return m_trackers.data(); }
    int Count() const { return (int)m_trackers.size(); }
    void SetModel(int trackerID, const Item &model);
    void SetPose(int trackerID, const DirectX::XMFLOAT4X4 &pose);
    void AddModel(const std::shared_ptr<scngrph::Model> &model)
    {
        m_trackers.push_back(model);
    }
};

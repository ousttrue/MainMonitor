#pragma once
#include <memory>
#include <vector>
#include "Model.h"

namespace scngrph
{
class Scene
{
    using Item = std::shared_ptr<scngrph::Model>;
    std::vector<Item> m_trackers;

public:
    Scene();
    const Item *GetModels(int *pCount) const
    {
        *pCount = (int)m_trackers.size();
        return m_trackers.data();
    }
    int Count() const { return (int)m_trackers.size(); }
    void SetModel(int trackerID, const Item &model);
    void SetPose(int trackerID, const DirectX::XMFLOAT4X4 &pose);
    void AddEmpty()
    {
        m_trackers.push_back(nullptr);
    }
    void AddModel(const std::shared_ptr<scngrph::Model> &model)
    {
        m_trackers.push_back(model);
    }
};
} // namespace scngrph
#pragma once
#include <memory>
#include <vector>
#include <string>
#include "Model.h"

namespace hierarchy
{
class Scene
{
    using Item = std::shared_ptr<hierarchy::Model>;
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
    void AddModel(const std::shared_ptr<hierarchy::Model> &model)
    {
        m_trackers.push_back(model);
    }
    void LoadFromPath(const std::string &path);
    void LoadFromPath(const std::wstring &path);
    void LoadGlbBytes(const uint8_t *p, int size);
};
} // namespace hierarchy
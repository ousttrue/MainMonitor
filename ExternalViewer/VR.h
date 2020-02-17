#pragma once
#include <openvr.h>
// #include <string>
// #include <vector>
#include <DirectXMath.h>
#include <iostream>
#include <functional>
// #include <list>
#include <unordered_map>

class Tracker
{
    enum class TrackerState
    {
        Unknown,
        ModelLoading,
        TextureLoading,
        Completed,
        Error,
    };
    int m_index;
    std::string m_modelName;
    TrackerState m_state{};
    vr::RenderModel_t *m_pModel = nullptr;
    vr::RenderModel_TextureMap_t *m_pTexture = nullptr;

    DirectX::XMFLOAT4X4 m_pose = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

public:
    Tracker(int index, const std::string &modelName)
        : m_index(index), m_modelName(modelName), m_state(TrackerState::ModelLoading)
    {
    }

    ~Tracker()
    {
        Release();
    }

    int Index() const { return m_index; }
    vr::RenderModel_t *Model() const { return m_pModel; }
    vr::RenderModel_TextureMap_t *Texture() const { return m_pTexture; }
    const DirectX::XMFLOAT4X4 &Matrix() const { return m_pose; }
    void Matrix(const DirectX::XMFLOAT4X4 &pose) { m_pose = pose; }

    void Release()
    {
        if (m_pModel)
        {
            vr::VRRenderModels()->FreeRenderModel(m_pModel);
            m_pModel = nullptr;
        }
        if (m_pTexture)
        {
            vr::VRRenderModels()->FreeTexture(m_pTexture);
            m_pTexture = nullptr;
        }
    }

    bool Update()
    {
        if (m_state == TrackerState::ModelLoading)
        {
            auto error = vr::VRRenderModels()->LoadRenderModel_Async(m_modelName.c_str(), &m_pModel);
            if (error == vr::VRRenderModelError_Loading)
            {
                std::cout << m_modelName << " model loading..." << std::endl;
                return false;
            }
            else if (error != vr::VRRenderModelError_None)
            {
                std::cout << m_modelName << "fail to model load: " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error);
                m_state = TrackerState::Error;
                return false;
            }

            std::cout << m_modelName << " model loaded" << std::endl;
            m_state = TrackerState::TextureLoading;
            return false;
        }
        else if (m_state == TrackerState::TextureLoading)
        {
            auto error = vr::VRRenderModels()->LoadTexture_Async(m_pModel->diffuseTextureId, &m_pTexture);
            if (error == vr::VRRenderModelError_Loading)
            {
                std::cout << m_modelName << " texture loading..." << std::endl;
                return false;
            }
            else if (error != vr::VRRenderModelError_None)
            {
                std::cout << m_modelName << "fail to texture load: " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error);
                m_state = TrackerState::Error;
                return false;
            }

            std::cout << m_modelName << " texture loaded" << std::endl;
            m_state = TrackerState::Completed;
            return true;
        }
        else
        {
            return false;
        }
    }
};

using OnModelLoad = std::function<void(int, const vr::RenderModel_t *, const vr::RenderModel_TextureMap_t *)>;
class VR
{
    vr::IVRSystem *m_system = nullptr;

    OnModelLoad m_callback;
    vr::TrackedDevicePose_t m_poses[vr::k_unMaxTrackedDeviceCount] = {};

public:
    std::unordered_map<int, std::unique_ptr<Tracker>> m_trackers;
    void SetCallback(const OnModelLoad &callback);
    bool Connect();
    void OnFrame();
    void StartLoadModel(int index);
};

#include "Window.h"
#include "Renderer.h"
#include <openvr.h>
#include <iostream>
#include <functional>
#include <list>
#include <unordered_map>
#include <DirectXMath.h>
using namespace DirectX;

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

    XMFLOAT4X4 m_pose = {
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
    const XMFLOAT4X4 &Matrix() const { return m_pose; }
    void Matrix(const XMFLOAT4X4 &pose) { m_pose = pose; }

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

static XMFLOAT4X4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
    XMFLOAT4X4 matrixObj(
        matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
        matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
        matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
        matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f);
    return matrixObj;
}

using OnModelLoad = std::function<void(int, const vr::RenderModel_t *, const vr::RenderModel_TextureMap_t *)>;
class OpenVR
{
    vr::IVRSystem *m_system = nullptr;

    OnModelLoad m_callback;
    vr::TrackedDevicePose_t m_poses[vr::k_unMaxTrackedDeviceCount] = {};

public:
    std::unordered_map<int, std::unique_ptr<Tracker>> m_trackers;

    ~OpenVR()
    {
    }

    void SetCallback(const OnModelLoad &callback)
    {
        m_callback = callback;
    }

    bool Connect()
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

    void
    OnFrame()
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

    void StartLoadModel(int index)
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

    {
        Renderer renderer;
        OpenVR vr;

        auto callback = [&renderer](int index,
                                    const vr::RenderModel_t *model,
                                    const vr::RenderModel_TextureMap_t *texture) {
        };

        vr.SetCallback(callback);

        if (!vr.Connect())
        {
            return 2;
        }

        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame();
            for (auto &kv : vr.m_trackers)
            {
                auto &index = kv.first;
                auto &tracker = kv.second;
                if (tracker->Update())
                {
                    // on load
                    // if (m_callback)
                    // {
                    //     m_callback(tracker->Index(), tracker->Model(), tracker->Texture());
                    // }
                    auto model = tracker->Model();
                    auto vertexStride = (int)sizeof(model->rVertexData[0]);
                    auto indexStride = (int)sizeof(model->rIndexData[0]);
                    renderer.AddModel(index,
                                      (uint8_t *)model->rVertexData, model->unVertexCount * vertexStride, vertexStride,
                                      (uint8_t *)model->rIndexData, model->unTriangleCount * indexStride, indexStride);
                }
                renderer.SetPose(index, &tracker->Matrix()._11);
            }

            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

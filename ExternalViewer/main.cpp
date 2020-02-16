#include "Window.h"
#include "Renderer.h"
#include <openvr.h>
#include <iostream>
#include <functional>
#include <list>

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
class OpenVR
{
    vr::IVRSystem *m_system = nullptr;

    std::list<std::unique_ptr<Tracker>> m_trackers;
    OnModelLoad m_callback;

public:
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
        for (auto &tracker : m_trackers)
        {
            if (tracker->Update())
            {
                if (m_callback)
                {
                    m_callback(tracker->Index(), tracker->Model(), tracker->Texture());
                }
            }
        }

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
        std::cout << "loadModel: " << modelName << std::endl;
        m_trackers.push_back(std::make_unique<Tracker>(index, modelName));
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
            auto vertexStride = (int)sizeof(model->rVertexData[0]);
            auto indexStride = (int)sizeof(model->rIndexData[0]);
            renderer.AddModel(index,
                              (uint8_t *)model->rVertexData, model->unVertexCount * vertexStride, vertexStride,
                              (uint8_t *)model->rIndexData, model->unTriangleCount * indexStride, indexStride);
        };

        vr.SetCallback(callback);

        if (!vr.Connect())
        {
            // return 2;
        }

        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame();
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

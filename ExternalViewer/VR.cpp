#include "VR.h"
#include "Model.h"
#include <iostream>

struct LoadTask
{
    int m_index;
    std::string m_modelName;
    enum class TrackerState
    {
        Unknown,
        ModelLoading,
        TextureLoading,
        Completed,
        Error,
    };
    TrackerState m_state{};
    vr::RenderModel_t *m_pModel = nullptr;
    vr::RenderModel_TextureMap_t *m_pTexture = nullptr;

public:
    LoadTask(int index, const std::string &modelName)
        : m_index(index), m_modelName(modelName), m_state(TrackerState::ModelLoading)
    {
    }

    ~LoadTask()
    {
        Release();
    }
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

    int Index() const { return m_index; }
    vr::RenderModel_t *RenderModel() const { return m_pModel; }
    vr::RenderModel_TextureMap_t *Texture() const { return m_pTexture; }

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

static DirectX::XMFLOAT4X4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
    DirectX::XMFLOAT4X4 matrixObj(
        matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
        matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
        matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
        matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f);
    return matrixObj;
}

bool VR::Connect()
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

void VR::OnFrame()
{
    // load task
    for (auto it = m_tasks.begin(); it != m_tasks.end();)
    {
        auto &task = *it;
        if (task->Update())
        {
            // tracker model loaded
            auto data = task->RenderModel();
            auto vertexStride = (int)sizeof(data->rVertexData[0]);
            auto indexStride = (int)sizeof(data->rIndexData[0]);

            auto model = Model::Create();
            model->SetVertices((uint8_t *)data->rVertexData, data->unVertexCount * vertexStride, vertexStride);
            model->SetIndices((uint8_t *)data->rIndexData, data->unTriangleCount * indexStride, indexStride);
            m_trackers[task->m_index] = model;
            it = m_tasks.erase(it);
        }
        else
        {
            ++it;
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

    // update pose
    m_system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0,
                                              m_poses, vr::k_unMaxTrackedDeviceCount);
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
    {
        auto tracker = m_trackers[i];
        if (m_poses[i].bPoseIsValid)
        {
            if (tracker)
            {
                auto pose = ConvertSteamVRMatrixToMatrix4(m_poses[i].mDeviceToAbsoluteTracking);
                tracker->Data.world = pose;
            }
        }
    }
}

void VR::StartLoadModel(int index)
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
    m_tasks.push_back(std::make_shared<LoadTask>(index, modelName));
}

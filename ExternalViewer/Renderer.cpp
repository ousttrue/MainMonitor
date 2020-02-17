#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>

std::string g_shaderSource =
#include "OpenVRRenderModel.hlsl"
    ;

using namespace d12u;

class Impl
{
    ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
    std::unordered_map<int, std::shared_ptr<Model>> m_models;
    std::unique_ptr<Uploader> m_uploader;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Heap> m_heap;

    d12u::ConstantBuffer<Camera::SceneConstantBuffer, 1> m_sceneConstant;
    d12u::ConstantBuffer<Model::ModelConstantBuffer, 64> m_modelConstant;

public:
    Impl()
        : m_queue(new CommandQueue),
          m_rt(new SwapChain(2)),
          m_uploader(new Uploader),
          m_pipeline(new Pipeline),
          m_camera(new Camera),
          m_heap(new Heap)
    {
    }

    void Initialize(HWND hwnd)
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter = GetHardwareAdapter(factory.Get());
        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));

        m_queue->Initialize(m_device);
        m_rt->Initialize(factory, m_queue->Get(), hwnd);
        m_uploader->Initialize(m_device);
        m_pipeline->Initialize(m_device, g_shaderSource, 2);
        m_sceneConstant.Initialize(m_device);
        m_modelConstant.Initialize(m_device);

        ConstantBufferBase *buffers[] = {
            &m_sceneConstant,
            &m_modelConstant,
        };

        m_heap->Initialize(m_device, buffers, 2, 64);
    }

    void OnFrame(HWND hwnd, const ScreenState &state)
    {
        if (!m_device)
        {
            // first time
            Initialize(hwnd);
        }

        m_uploader->Update(m_device);

        // update
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_rt->Resize(m_queue->Get(),
                         hwnd, state.Width, state.Height);
        }
        if (m_camera->OnFrame(state, m_lastState))
        {
            *m_sceneConstant.Get(0) = m_camera->Data;
            m_sceneConstant.CopyToGpu();
        }
        m_modelConstant.CopyToGpu();
        m_lastState = state;

        // command
        auto commandList = m_pipeline->Reset();
        float color[] = {
            0,
            0,
            0,
            1.0f,
        };
        auto &rt = m_rt->Begin(commandList->Get(), color);

        // scene
        {
            ID3D12DescriptorHeap *ppHeaps[] = {m_heap->Get()};
            commandList->Get()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        }

        // model
        for (auto &kv : m_models)
        {
            commandList->Get()->SetGraphicsRootDescriptorTable(0, m_heap->GpuHandle(kv.first));
            auto mesh = GetOrCreate(kv.second);
            mesh->Command(commandList);
        }

        m_rt->End(commandList->Get(), rt);
        auto callbacks = commandList->Close();

        // execute
        m_queue->Execute(commandList->Get());
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }

    void AddModel(int index, const std::shared_ptr<Model> &model)
    {
        m_models.insert(std::make_pair(index, model));
    }

    std::unordered_map<std::shared_ptr<Model>, std::shared_ptr<Mesh>> m_modelMeshMap;
    std::shared_ptr<Mesh> GetOrCreate(const std::shared_ptr<Model> &model)
    {
        auto found = m_modelMeshMap.find(model);
        if (found != m_modelMeshMap.end())
        {
            return found->second;
        }

        auto mesh = std::make_shared<Mesh>();

        {
            auto vertices = ResourceItem::CreateDefault(m_device, model->VerticesByteLength());
            mesh->VertexBuffer(vertices);
            m_uploader->EnqueueUpload(vertices, model->Vertices(), model->VerticesByteLength(), model->VertexStride());
        }

        {
            auto indices = ResourceItem::CreateDefault(m_device, model->IndicesByteLength());
            mesh->IndexBuffer(indices);
            m_uploader->EnqueueUpload(indices, model->Indices(), model->IndicesByteLength(), model->IndexStride());
        }

        m_modelMeshMap.insert(std::make_pair(model, mesh));
        return mesh;
    }

    void SetPose(int index, const DirectX::XMFLOAT4X4 &m)
    {
        m_modelConstant.Get(index)->world = m;
    }
};

Renderer::Renderer()
    : m_impl(new Impl)
{
}

Renderer::~Renderer()
{
    delete m_impl;
}

void Renderer::OnFrame(void *hwnd, const ScreenState &state)
{
    m_impl->OnFrame((HWND)hwnd, state);
}

void Renderer::AddModel(int index,
                        const uint8_t *vertices, int verticesByteLength, int vertexStride,
                        const uint8_t *indices, int indicesByteLength, int indexStride)
{
    auto model = Model::Create();
    model->SetVeritces(vertices, verticesByteLength, vertexStride);
    model->SetIndices(indices, indicesByteLength, indexStride);
    m_impl->AddModel(index, model);
}

void Renderer::SetPose(int index, const float *matrix)
{
    m_impl->SetPose(index, *((const DirectX::XMFLOAT4X4 *)matrix));
}

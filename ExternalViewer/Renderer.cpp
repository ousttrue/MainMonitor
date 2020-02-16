#include "Renderer.h"
#include "Model.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>

using namespace d12u;

class Impl
{
    ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
    std::unique_ptr<CommandList> m_command;
    std::unordered_map<int, std::shared_ptr<Model>> m_models;

public:
    Impl()
        : m_queue(new CommandQueue),
          m_rt(new SwapChain(2)),
          m_command(new CommandList)
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
        m_command->Initialize(m_device, nullptr);
    }

    void OnFrame(HWND hwnd, const ScreenState &state)
    {
        // update
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_rt->Resize(m_queue->Get(),
                         hwnd, state.Width, state.Height);
        }
        m_lastState = state;

        float intPart;
        auto value = modf(m_lastState.Time * 0.001f, &intPart);

        // command
        m_command->Reset(nullptr);
        float color[] = {
            value,
            value,
            value,
            1.0f,
        };
        auto &rt = m_rt->Begin(m_command->Get(), color);

        for(auto kv: m_models)
        {
            // GetOrCreate(kv);
        }

        m_rt->End(m_command->Get(), rt);
        auto callbacks = m_command->Close();

        // execute
        m_queue->Execute(m_command->Get());
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }

    void AddModel(int index, const std::shared_ptr<Model> &model)
    {
        m_models.insert(std::make_pair(index, model));
    }
};

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    if (m_impl)
        delete m_impl;
}

void Renderer::OnFrame(void *hwnd, const ScreenState &state)
{
    if (!m_impl)
    {
        // first time
        m_impl = new Impl();
        m_impl->Initialize((HWND)hwnd);
    }
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

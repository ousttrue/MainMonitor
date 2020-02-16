#include "Renderer.h"
#include "MyD3D12Helper.h"
#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <math.h>

class NonCopyable
{
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

protected:
    NonCopyable() = default;
};

class CommandQueue : NonCopyable
{
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_nextFenceValue = 0;
    HANDLE m_fenceEvent = NULL;

public:
    const ComPtr<ID3D12CommandQueue> &Get() const { return m_queue; }

    ~CommandQueue()
    {
        SyncFence();
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
        }
    }

    void Initialize(const ComPtr<ID3D12Device> &device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = type,
        };
        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));

        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_nextFenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    void Execute(const ComPtr<ID3D12CommandList> &commandList)
    {
        ID3D12CommandList *list[] = {
            commandList.Get(),
        };
        m_queue->ExecuteCommandLists(_countof(list), list);
    }

    UINT64 Signal()
    {
        // Signal and increment the fence value.
        const UINT64 fence = m_nextFenceValue++;
        ThrowIfFailed(m_queue->Signal(m_fence.Get(), fence));
        return fence;
    }

    UINT64 CurrentValue() const
    {
        return m_fence->GetCompletedValue();
    }

    using CallbackList = std::list<std::function<void()>>;
    void SyncFence(const CallbackList &callbacks = CallbackList())
    {
        auto fence = Signal();

        // Wait until the previous frame is finished.
        if (CurrentValue() < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        for (auto &callback : callbacks)
        {
            callback();
        }
    }
};

struct RenderTarget : NonCopyable
{
    ComPtr<ID3D12Resource> Resource;
    D3D12_CPU_DESCRIPTOR_HANDLE RTV{};
    Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencil;
    D3D12_CPU_DESCRIPTOR_HANDLE DSV;

    void Initialzie(const ComPtr<IDXGISwapChain3> &swapChain,
                    int i,
                    const ComPtr<ID3D12Device> &device,
                    const D3D12_CPU_DESCRIPTOR_HANDLE &rtv,
                    const D3D12_CPU_DESCRIPTOR_HANDLE &dsv)
    {
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&Resource)));
        device->CreateRenderTargetView(Resource.Get(), nullptr, rtv);
        RTV = rtv;

        // Create depth
        auto depthDesc = Resource->GetDesc();
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_HEAP_PROPERTIES prop{
            .Type = D3D12_HEAP_TYPE_DEFAULT,
        };
        D3D12_CLEAR_VALUE clear{DXGI_FORMAT_D32_FLOAT, 1.0f, 0};
        device->CreateCommittedResource(&prop,
                                        D3D12_HEAP_FLAG_NONE,
                                        &depthDesc,
                                        D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                        &clear,
                                        IID_PPV_ARGS(&DepthStencil));
        device->CreateDepthStencilView(DepthStencil.Get(), nullptr, dsv);
        DSV = dsv;
    }

    void Release()
    {
        Resource.Reset();
        DepthStencil.Reset();
    }
};

class SwapChainRenderTarget : NonCopyable
{
    ComPtr<IDXGISwapChain3> m_swapChain;
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    std::vector<RenderTarget> m_backbuffers;

private:
    void Create(
        const ComPtr<IDXGIFactory4> &factory,
        const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width = 0, int height = 0)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
            {
                .Width = (UINT)width,
                .Height = (UINT)height,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {
                    .Count = 1,
                },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = (UINT)m_backbuffers.size(),
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            };
        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            commandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain));

        // This sample does not support fullscreen transitions.
        ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
        ThrowIfFailed(swapChain.As(&m_swapChain));

        m_swapChain->GetDesc1(&swapChainDesc);
        m_viewport = {
            .Width = (float)swapChainDesc.Width,
            .Height = (float)swapChainDesc.Height,
            .MinDepth = 0,
            .MaxDepth = 1.0f,
        };
        m_scissorRect = {
            .right = (LONG)swapChainDesc.Width,
            .bottom = (LONG)swapChainDesc.Height,
        };
    }

public:
    SwapChainRenderTarget(int backbufferCount)
        : m_backbuffers(backbufferCount)
    {
    }

    void Initialize(const ComPtr<IDXGIFactory4> &factory,
                    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd)
    {
        Create(factory, commandQueue, hwnd);
    }

    void Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height)
    {
        // backup factory
        ComPtr<IDXGIFactory4> factory;
        m_swapChain->GetParent(IID_PPV_ARGS(&factory));

        ////////////////////
        // release !
        ////////////////////
        for (auto &backbuffer : m_backbuffers)
        {
            backbuffer.Release();
        }
        m_swapChain.Reset();

        Create(factory, commandQueue, hwnd, width, height);
    }

    RenderTarget *Begin(
        const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor)
    {
        ComPtr<ID3D12Device> device;
        commandList->GetDevice(IID_PPV_ARGS(&device));

        if (!m_rtvHeap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = (UINT)m_backbuffers.size(),
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            };
            ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)));
        }
        if (!m_dsvHeap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                .NumDescriptors = (UINT)m_backbuffers.size(),
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            };
            ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)));
        }
        // Create frame resources.
        if (!m_backbuffers[0].Resource)
        {
            auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
            int i = 0;
            for (auto &backbuffer : m_backbuffers)
            {
                backbuffer.Initialzie(m_swapChain, i++, device, rtv, dsv);
                rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            }
        }

        auto frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        auto backbuffer = &m_backbuffers[frameIndex];

        commandList->RSSetViewports(1, &m_viewport);
        commandList->RSSetScissorRects(1, &m_scissorRect);
        D3D12_RESOURCE_BARRIER barrier{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Transition{
                .pResource = backbuffer->Resource.Get(),
                .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
                .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
            },
        };
        commandList->ResourceBarrier(1, &barrier);

        commandList->OMSetRenderTargets(1, &backbuffer->RTV, FALSE, &backbuffer->DSV);
        commandList->ClearRenderTargetView(backbuffer->RTV, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(backbuffer->DSV, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);

        return backbuffer;
    }

    void End(const ComPtr<ID3D12GraphicsCommandList> &commandList, const RenderTarget *rt)
    {
        D3D12_RESOURCE_BARRIER barrier{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Transition{
                .pResource = rt->Resource.Get(),
                .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
                .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
            },
        };
        commandList->ResourceBarrier(1, &barrier);
    }

    void Present()
    {
        ThrowIfFailed(m_swapChain->Present(1, 0));
    }
};

class CommandList : NonCopyable
{
    using OnCompletedFunc = std::function<void()>;

    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    std::list<OnCompletedFunc> m_callbacks;

public:
    const ComPtr<ID3D12GraphicsCommandList> &Get() const { return m_commandList; }

    void Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12PipelineState> &ps,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_commandAllocator)));

        // Create the command list.
        ThrowIfFailed(device->CreateCommandList(0, type, m_commandAllocator.Get(), ps.Get(), IID_PPV_ARGS(&m_commandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        ThrowIfFailed(m_commandList->Close());
    }

    void Reset(const ComPtr<ID3D12PipelineState> &ps)
    {
        ThrowIfFailed(m_commandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command
        // list, that command list can then be reset at any time and must be before
        // re-recording.
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), ps.Get()));
    }

    std::list<OnCompletedFunc> Close()
    {
        m_commandList->Close();

        auto callbacks = m_callbacks;
        m_callbacks.clear();

        return callbacks;
    }
};

class Impl
{
    ScreenState m_lastState = {};
    ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChainRenderTarget> m_rt;
    std::unique_ptr<CommandList> m_command;

public:
    Impl()
        : m_queue(new CommandQueue),
          m_rt(new SwapChainRenderTarget(2)),
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
        auto rt = m_rt->Begin(m_command->Get(), color);
        // TODO: Scene
        m_rt->End(m_command->Get(), rt);
        auto callbacks = m_command->Close();

        // execute
        m_queue->Execute(m_command->Get());
        m_rt->Present();
        m_queue->SyncFence(callbacks);
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

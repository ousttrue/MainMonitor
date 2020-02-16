#include "SwapChain.h"
#include "RenderTarget.h"

namespace d12u
{
SwapChain::SwapChain(int backbufferCount)
{
    for(int i=0; i<backbufferCount; ++i)
    {
        m_backbuffers.push_back(std::make_unique<RenderTarget>());
    }
}

void SwapChain::Create(
    const ComPtr<IDXGIFactory4> &factory,
    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height)
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

void SwapChain::Initialize(const ComPtr<IDXGIFactory4> &factory,
                           const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd)
{
    Create(factory, commandQueue, hwnd);
}

void SwapChain::Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height)
{
    // backup factory
    ComPtr<IDXGIFactory4> factory;
    m_swapChain->GetParent(IID_PPV_ARGS(&factory));

    ////////////////////
    // release !
    ////////////////////
    for (auto &backbuffer : m_backbuffers)
    {
        backbuffer->Release();
    }
    m_swapChain.Reset();

    Create(factory, commandQueue, hwnd, width, height);
}

std::unique_ptr<RenderTarget> &SwapChain::Begin(
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
    if (!m_backbuffers[0]->Resource)
    {
        auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        int i = 0;
        for (auto &backbuffer : m_backbuffers)
        {
            backbuffer->Initialzie(m_swapChain, i++, device, rtv, dsv);
            rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }
    }

    auto frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    auto &backbuffer = m_backbuffers[frameIndex];

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

void SwapChain::End(const ComPtr<ID3D12GraphicsCommandList> &commandList, const std::unique_ptr<RenderTarget> &rt)
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

void SwapChain::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0));
}

} // namespace d12u
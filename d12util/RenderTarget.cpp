#include "RenderTarget.h"

namespace d12u
{
void RenderTargetChain::Initialize(const ComPtr<IDXGISwapChain3> &swapChain,
                                   const ComPtr<ID3D12Device> &device,
                                   UINT frameCount)
{
    m_resources.resize(frameCount);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    ThrowIfFailed(swapChain->GetDesc1(&swapChainDesc));
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

    if (!m_rtvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = (UINT)m_resources.size(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)));
    }
    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    if (!m_dsvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = (UINT)m_resources.size(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)));
    }
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    int i = 0;
    for (auto &resource : m_resources)
    {
        ThrowIfFailed(swapChain->GetBuffer(i++, IID_PPV_ARGS(&resource.renderTarget)));
        device->CreateRenderTargetView(resource.renderTarget.Get(), nullptr, rtv);
        rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create depth
        auto depthDesc = resource.renderTarget->GetDesc();
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
                                        IID_PPV_ARGS(&resource.depthStencil));
        device->CreateDepthStencilView(resource.depthStencil.Get(), nullptr, dsv);
        dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }
}

void RenderTargetChain::Begin(UINT frameIndex,
                              const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor)
{
    ComPtr<ID3D12Device> device;
    commandList->GetDevice(IID_PPV_ARGS(&device));

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = m_resources[frameIndex].renderTarget.Get(),
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        },
    };
    commandList->ResourceBarrier(1, &barrier);

    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * frameIndex;
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV) * frameIndex;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
}

void RenderTargetChain::End(UINT frameIndex,
                            const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = m_resources[frameIndex].renderTarget.Get(),
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        },
    };
    commandList->ResourceBarrier(1, &barrier);
}

} // namespace d12u

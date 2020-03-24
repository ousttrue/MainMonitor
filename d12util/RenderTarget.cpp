#include "RenderTarget.h"

namespace d12u
{

void RenderTargetResources::CreateDepthResource(const ComPtr<ID3D12Device> &device)
{
    auto depthDesc = renderTarget->GetDesc();
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
                                    IID_PPV_ARGS(&depthStencil));
}

void RenderTargetChain::CreateHeap(const ComPtr<ID3D12Device> &device)
{
    if (!m_rtvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = (UINT)m_resources.size(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)));
    }

    if (!m_dsvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = (UINT)m_resources.size(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)));
    }
}

void RenderTargetChain::Initialize(const ComPtr<IDXGISwapChain3> &swapChain,
                                   const ComPtr<ID3D12Device> &device,
                                   UINT frameCount)
{
    m_resources.resize(frameCount);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    ThrowIfFailed(swapChain->GetDesc1(&swapChainDesc));
    Resize(swapChainDesc.Width, swapChainDesc.Height);

    CreateHeap(device);
    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    int i = 0;
    for (auto &resource : m_resources)
    {
        // RTV resource from swapchain
        ThrowIfFailed(swapChain->GetBuffer(i++, IID_PPV_ARGS(&resource.renderTarget)));
        device->CreateRenderTargetView(resource.renderTarget.Get(), nullptr, rtv);
        rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // DSV resource that has same size with RTV
        resource.CreateDepthResource(device);
        device->CreateDepthStencilView(resource.depthStencil.Get(), nullptr, dsv);
        dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    m_isSwapchain = true;
}

void RenderTargetChain::Initialize(UINT width, UINT height,
                                   const ComPtr<ID3D12Device> &device,
                                   UINT frameCount)
{
    m_resources.resize(frameCount);

    CreateHeap(device);
    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    int i = 0;
    for (auto &resource : m_resources)
    {
        // RTV resource
        // auto depthDesc = renderTarget->GetDesc();
        D3D12_RESOURCE_DESC desc{
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = width,
            .Height = height,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {1, 0},
            .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        };
        D3D12_HEAP_PROPERTIES prop{
            .Type = D3D12_HEAP_TYPE_DEFAULT,
        };
        D3D12_CLEAR_VALUE clear{
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Color = {
                0,
                0,
                0,
                0,
            }};
        ThrowIfFailed(device->CreateCommittedResource(&prop,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      &clear,
                                                      IID_PPV_ARGS(&resource.renderTarget)));
        device->CreateRenderTargetView(resource.renderTarget.Get(), nullptr, rtv);
        rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // DSV resource that has same size with RTV
        resource.CreateDepthResource(device);
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

    if (m_isSwapchain)
    {
        D3D12_RESOURCE_BARRIER barrier{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Transition{
                .pResource = m_resources[frameIndex].renderTarget.Get(),
                .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
                .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
            },
        };
        commandList->ResourceBarrier(1, &barrier);
    }

    auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * frameIndex;
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    dsv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV) * frameIndex;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    if (clearColor)
    {
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
}

void RenderTargetChain::End(UINT frameIndex,
                            const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    if (m_isSwapchain)
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
}

} // namespace d12u

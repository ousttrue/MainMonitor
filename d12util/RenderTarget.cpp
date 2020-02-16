#include "RenderTarget.h"

namespace d12u
{
D3D12_CPU_DESCRIPTOR_HANDLE DSV;

void RenderTarget::Initialzie(const ComPtr<IDXGISwapChain3> &swapChain,
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

void RenderTarget::Release()
{
    Resource.Reset();
    DepthStencil.Reset();
}

} // namespace d12u

#pragma once
#include "Helper.h"
#include <vector>
#include <memory>

namespace d12u
{

struct RenderTargetResources
{
    ComPtr<ID3D12Resource> renderTarget;
    ComPtr<ID3D12Resource> depthStencil;
};

class RenderTargetChain
{
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    std::vector<RenderTargetResources> m_resources;

public:
    void Release()
    {
        m_resources.clear();
    }

    void Initialize(const ComPtr<IDXGISwapChain3> &swapChain,
                    const ComPtr<ID3D12Device> &device,
                    UINT frameCount);

    void Begin(UINT frameIndex,
               const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor);
    void End(UINT frameIndex, const ComPtr<ID3D12GraphicsCommandList> &commandList);
};


} // namespace d12u

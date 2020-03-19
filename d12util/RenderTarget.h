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
    void CreateDepthResource(const ComPtr<ID3D12Device> &device);
};

class RenderTargetChain
{
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    void SetSize(UINT width, UINT height)
    {
        m_viewport = {
            .Width = (float)width,
            .Height = (float)height,
            .MinDepth = 0,
            .MaxDepth = 1.0f,
        };
        m_scissorRect = {
            .right = (LONG)width,
            .bottom = (LONG)height,
        };
    }

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    void CreateHeap(const ComPtr<ID3D12Device> &device);

    std::vector<RenderTargetResources> m_resources;

public:
    void Release()
    {
        m_resources.clear();
    }

    void Initialize(const ComPtr<IDXGISwapChain3> &swapChain,
                    const ComPtr<ID3D12Device> &device,
                    UINT frameCount);

    void Initialize(UINT width, UINT height,
                    const ComPtr<ID3D12Device> &device,
                    UINT frameCount);

    void Begin(UINT frameIndex,
               const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor);
    void End(UINT frameIndex, const ComPtr<ID3D12GraphicsCommandList> &commandList);

    RenderTargetResources *Resource(UINT frameIndex)
    {
        return &m_resources[frameIndex];
    }
};

} // namespace d12u

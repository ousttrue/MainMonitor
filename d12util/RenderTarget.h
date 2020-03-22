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

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    void CreateHeap(const ComPtr<ID3D12Device> &device);

    std::vector<RenderTargetResources> m_resources;

    bool m_isSwapchain = false;

public:
    void Release()
    {
        m_resources.clear();
    }

    bool Resize(UINT width, UINT height)
    {
        if (m_viewport.Width == width && m_viewport.Height == height)
        {
            return false;
        }

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
        return true;
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
        if (frameIndex >= m_resources.size())
        {
            return nullptr;
        }
        return &m_resources[frameIndex];
    }
};

} // namespace d12u

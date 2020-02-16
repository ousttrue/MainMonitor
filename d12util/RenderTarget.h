#pragma once
#include "Helper.h"

namespace d12u
{
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
                    const D3D12_CPU_DESCRIPTOR_HANDLE &dsv);
    void Release();
};
} // namespace d12u

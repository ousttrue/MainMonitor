#pragma once
#include "Helper.h"
#include <vector>
#include <memory>

namespace d12u
{
class SwapChain : NonCopyable
{
    ComPtr<IDXGISwapChain3> m_swapChain;
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    std::vector<std::unique_ptr<struct RenderTarget>> m_backbuffers;

private:
    void Create(
        const ComPtr<IDXGIFactory4> &factory,
        const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width = 0, int height = 0);

public:
    SwapChain(int backbufferCount);
    int BufferCount() const { return (int)m_backbuffers.size(); }
    void Initialize(const ComPtr<IDXGIFactory4> &factory,
                    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd);
    void Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height);
    std::unique_ptr<struct RenderTarget> &Begin(
        const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor);
    void End(const ComPtr<ID3D12GraphicsCommandList> &commandList, const std::unique_ptr<RenderTarget> &rt);
    void Present();
};

} // namespace d12u

#pragma once
#include "Helper.h"
#include <vector>
#include <memory>

namespace d12u
{

class SwapChain : NonCopyable
{
    ComPtr<IDXGISwapChain3> m_swapChain;

private:
    void Create(
        const ComPtr<IDXGIFactory4> &factory,
        const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, UINT bufferCount, int width = 0, int height = 0);

public:
    const ComPtr<IDXGISwapChain3> &Get() const { return m_swapChain; }
    void Initialize(const ComPtr<IDXGIFactory4> &factory,
                    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, UINT bufferCount)
    {
        Create(factory, commandQueue, hwnd, bufferCount);
    }
    UINT CurrentFrameIndex() const
    {
        return m_swapChain->GetCurrentBackBufferIndex();
    }
    void Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, UINT frameCount, int width, int height);
    void Present();
};

} // namespace d12u

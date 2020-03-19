#include "SwapChain.h"
#include "RenderTarget.h"

namespace d12u
{

void SwapChain::Create(
    const ComPtr<IDXGIFactory4> &factory,
    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, UINT bufferCount, int width, int height)
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
            .BufferCount = bufferCount,
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
}

void SwapChain::Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, UINT frameCount, int width, int height)
{
    // backup factory
    ComPtr<IDXGIFactory4> factory;
    m_swapChain->GetParent(IID_PPV_ARGS(&factory));

    ////////////////////
    // release !
    ////////////////////

    m_swapChain.Reset();

    Create(factory, commandQueue, hwnd, frameCount, width, height);
}

void SwapChain::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0));
}

} // namespace d12u
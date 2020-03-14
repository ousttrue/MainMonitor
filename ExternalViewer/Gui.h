#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <ScreenState.h>
#include <list>
#include <memory>

class Gui
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class ImGuiDX12> m_dx12;
    std::unique_ptr<struct ExampleAppLog> m_logger;

public:
    Gui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd);
    ~Gui();
    void BeginFrame(const screenstate::ScreenState &state);
    void EndFrame(const ComPtr<ID3D12GraphicsCommandList> &commandList);

    void Logger();
};

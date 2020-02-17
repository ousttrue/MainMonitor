#pragma once
#include <d3dcompiler.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <imgui.h>

class ImGuiDX12
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // DirectX data
    ComPtr<ID3D12Device> g_pd3dDevice;
    ComPtr<ID3D12RootSignature> g_pRootSignature;
    ComPtr<ID3D12PipelineState> g_pPipelineState;
    DXGI_FORMAT g_RTVFormat = DXGI_FORMAT_UNKNOWN;
    ComPtr<ID3D12Resource> g_pFontTextureResource = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE g_hFontSrvCpuDescHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE g_hFontSrvGpuDescHandle = {};

public:
    struct FrameResources
    {
        ComPtr<ID3D12Resource> IndexBuffer;
        ComPtr<ID3D12Resource> VertexBuffer;
        int IndexBufferSize = 10000;
        int VertexBufferSize = 5000;
    };

private:
    std::vector<FrameResources> g_pFrameResources;
    UINT g_numFramesInFlight = 0;
    UINT g_frameIndex = UINT_MAX;

public:
    bool Init(ID3D12Device *device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap *cbv_srv_heap,
              D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);
    void NewFrame(ID3D12Device *device);
    void RenderDrawData(ImDrawData *draw_data, ID3D12GraphicsCommandList *ctx);
private:
    void InvalidateDeviceObjects();
};

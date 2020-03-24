#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class FrameResources
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_IndexBuffer;
    ComPtr<ID3D12Resource> m_VertexBuffer;
    int m_IndexBufferSize = 0;
    int m_VertexBufferSize = 0;

public:
    void SetupRenderState(struct ImDrawData *draw_data, const ComPtr<ID3D12GraphicsCommandList> &ctx);
    void EnsureVertexBufferSize(const ComPtr<ID3D12Device> &device, int size);
    void EnsureIndexBufferSize(const ComPtr<ID3D12Device> &device, int size);
    void MapCopyUnmap(struct ImDrawData *draw_data);
};

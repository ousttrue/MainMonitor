#pragma once

class ImGuiDX12
{
    class ImGuiDX12Impl *m_impl = nullptr;

public:
    ImGuiDX12();
    ~ImGuiDX12();
    void Initialize(struct ID3D12Device *device, int bufferCount);
    void RenderDrawData(struct ID3D12GraphicsCommandList *ctx, struct ImDrawData *draw_data);
    size_t GetOrCreateTexture(struct ID3D12Device *device,
                              struct ID3D12Resource *resource);
    void Remove(ID3D12Resource *resource);
};

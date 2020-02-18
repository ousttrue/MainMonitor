#include "ImGuiDX12FrameResources.h"
#include <imgui.h>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct VERTEX_CONSTANT_BUFFER
{
    float mvp[4][4];
};

static ComPtr<ID3D12Resource> CreateVertexBuffer(const ComPtr<ID3D12Device> &device, UINT byteSize)
{
    D3D12_HEAP_PROPERTIES props{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = byteSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    ComPtr<ID3D12Resource> vertexBuffer;
    if (FAILED(device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            IID_PPV_ARGS(&vertexBuffer))))
    {
        throw;
    }
    return vertexBuffer;
}

static ComPtr<ID3D12Resource> CreateIndexBuffer(const ComPtr<ID3D12Device> &device, UINT byteSize)
{
    D3D12_HEAP_PROPERTIES props{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = byteSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    ComPtr<ID3D12Resource> indexBuffer;
    if (FAILED(device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               NULL,
                                               IID_PPV_ARGS(&indexBuffer))))
    {
        throw;
    }
    return indexBuffer;
}

void FrameResources::SetupRenderState(struct ImDrawData *draw_data, const ComPtr<ID3D12GraphicsCommandList> &ctx)
{
    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
            {
                {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
                {0.0f, 0.0f, 0.5f, 0.0f},
                {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
            };
        memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
    }

    // Setup viewport
    D3D12_VIEWPORT vp{
        .Width = draw_data->DisplaySize.x,
        .Height = draw_data->DisplaySize.y,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    ctx->RSSetViewports(1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
    vbv.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress() + offset;
    vbv.SizeInBytes = m_VertexBufferSize * stride;
    vbv.StrideInBytes = stride;
    ctx->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
    ibv.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    ibv.SizeInBytes = m_IndexBufferSize * sizeof(ImDrawIdx);
    ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    ctx->IASetIndexBuffer(&ibv);

    ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

    // Setup blend factor
    const float blend_factor[4] = {0.f, 0.f, 0.f, 0.f};
    ctx->OMSetBlendFactor(blend_factor);
}

void FrameResources::EnsureVertexBufferSize(const ComPtr<ID3D12Device> &device, int size)
{
    if (m_VertexBuffer == NULL || m_VertexBufferSize < size)
    {
        m_VertexBufferSize = size + 5000;
        m_VertexBuffer = CreateVertexBuffer(device, m_VertexBufferSize * sizeof(ImDrawVert));
    }
}

void FrameResources::EnsureIndexBufferSize(const ComPtr<ID3D12Device> &device, int size)
{
    if (m_IndexBuffer == NULL || m_IndexBufferSize < size)
    {
        m_IndexBufferSize = size + 10000;
        m_IndexBuffer = CreateIndexBuffer(device, m_IndexBufferSize * sizeof(ImDrawIdx));
    }
}

void FrameResources::MapCopyUnmap(struct ImDrawData *draw_data)
{
    D3D12_RANGE range{};
    void *vtx_resource;
    if (m_VertexBuffer->Map(0, &range, &vtx_resource) != S_OK)
        return;
    void *idx_resource;
    if (m_IndexBuffer->Map(0, &range, &idx_resource) != S_OK)
        return;
    ImDrawVert *vtx_dst = (ImDrawVert *)vtx_resource;
    ImDrawIdx *idx_dst = (ImDrawIdx *)idx_resource;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    m_VertexBuffer->Unmap(0, &range);
    m_IndexBuffer->Unmap(0, &range);
}

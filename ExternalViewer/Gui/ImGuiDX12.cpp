#include "ImGuiDX12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <unordered_map>
#include <imgui.h>
#include "ImGuiDX12FrameResources.h"
#include <plog/Log.h>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#include "ImGuiDX12Impl.h"

static const char *g_vertexShader =
#include "ImGuiDX12.vs"
    ;

static const char *g_pixelShader =
#include "ImGuiDX12.ps"
    ;

class ImGuiDX12Impl
{
    ComPtr<ID3D12RootSignature> m_pRootSignature;
    ComPtr<ID3D12PipelineState> m_pPipelineState;
    ComPtr<ID3D12DescriptorHeap> m_pHeap;
    UINT m_increment = 0;
    // ComPtr<ID3D12Resource> m_pFontTextureResource;
    // D3D12_CPU_DESCRIPTOR_HANDLE m_hFontSrvCpuDescHandle = {};
    // D3D12_GPU_DESCRIPTOR_HANDLE m_hFontSrvGpuDescHandle = {};

    std::vector<FrameResources> m_frames;
    UINT m_frameIndex = UINT_MAX;

    std::unordered_map<ID3D12Resource *, size_t> m_textureDescriptorMap;
    std::vector<ComPtr<ID3D12Resource>> m_descriptors;

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandle(ImTextureID index)
    {
        auto handle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += (size_t)index * m_increment;
        return handle;
    }

public:
    ImGuiDX12Impl(int bufferCount)
        : m_frames(bufferCount)
    {
    }

    void Remove(ID3D12Resource *resource)
    {
        m_textureDescriptorMap.erase(resource);
    }

    size_t GetOrCreateTexture(ID3D12Device *device,
                              ID3D12Resource *resource)
    {
        auto found = m_textureDescriptorMap.find(resource);
        if (found != m_textureDescriptorMap.end())
        {
            return found->second;
        }

        // Create texture view
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = 1,
            },
        };

        auto index = m_textureDescriptorMap.size();
        // LOGD << "imgui: new texture#" << index;

        auto handle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * m_increment;
        device->CreateShaderResourceView(resource,
                                         &srvDesc, handle);

        m_descriptors.push_back(resource);
        m_textureDescriptorMap.insert(std::make_pair(resource, index));

        return index;
    }

    void Initialize(ID3D12Device *device)
    {
        // Setup back-end capabilities flags
        ImGuiIO &io = ImGui::GetIO();
        io.BackendRendererName = "imgui_impl_dx12";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        {
            // pipeline
            m_pRootSignature = CreateRootSignature(device);
            if (!m_pRootSignature)
            {
                throw;
            }
            m_pPipelineState = CreatePipelineState(device, m_pRootSignature, g_vertexShader, g_pixelShader);
            if (!m_pPipelineState)
            {
                throw;
            }
        }

        {
            // heap
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 128,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            };
            if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap))))
            {
                throw;
            }
            m_increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        {
            // font texture
            auto pFontTextureResource = CreateFontsTexture(device);
            auto viewIndex = GetOrCreateTexture(device, pFontTextureResource.Get());
            // Store our identifier
            io.Fonts->TexID = (ImTextureID)viewIndex;
        }
    }

    void RenderDrawData(ID3D12GraphicsCommandList *ctx, ImDrawData *draw_data)
    {
        ComPtr<ID3D12Device> device;
        ctx->GetDevice(IID_PPV_ARGS(&device));

        if (!m_pRootSignature)
        {
            Initialize(device.Get());
        }

        // FIXME: I'm assuming that this only gets called once per frame!
        // If not, we can't just re-allocate the IB or VB, we'll have to do a proper allocator.
        m_frameIndex = m_frameIndex + 1;
        auto fr = &m_frames[m_frameIndex % m_frames.size()];

        // Avoid rendering when minimized
        if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        {
            return;
        }

        // Create and grow vertex/index buffers if needed
        fr->EnsureVertexBufferSize(device, draw_data->TotalVtxCount);
        fr->EnsureIndexBufferSize(device, draw_data->TotalIdxCount);

        //
        // Upload vertex/index data into a single contiguous GPU buffer
        //
        fr->MapCopyUnmap(draw_data);

        //
        // build commandlist
        //
        ctx->SetPipelineState(m_pPipelineState.Get());
        ctx->SetGraphicsRootSignature(m_pRootSignature.Get());
        ctx->SetDescriptorHeaps(1, m_pHeap.GetAddressOf());
        fr->SetupRenderState(draw_data, ctx);
        ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        ImVec2 clip_off = draw_data->DisplayPos;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    {
                        fr->SetupRenderState(draw_data, ctx);
                    }
                    else
                    {
                        pcmd->UserCallback(cmd_list, pcmd);
                    }
                }
                else
                {
                    // Apply Scissor, Bind texture, Draw
                    const D3D12_RECT r = {(LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y)};
                    ctx->SetGraphicsRootDescriptorTable(1, GetHandle(pcmd->TextureId));
                    ctx->RSSetScissorRects(1, &r);
                    ctx->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }

private:
};

ImGuiDX12::ImGuiDX12()
{
}

ImGuiDX12::~ImGuiDX12()
{
    if (m_impl)
    {
        delete m_impl;
    }
}

void ImGuiDX12::Initialize(struct ID3D12Device *device, int bufferCount)
{
    if (m_impl)
    {
        delete m_impl;
    }
    m_impl = new ImGuiDX12Impl(bufferCount);
    m_impl->Initialize(device);
}

void ImGuiDX12::RenderDrawData(ID3D12GraphicsCommandList *ctx, struct ImDrawData *draw_data)
{
    m_impl->RenderDrawData(ctx, draw_data);
}

size_t ImGuiDX12::GetOrCreateTexture(struct ID3D12Device *device,
                                     struct ID3D12Resource *resource)
{
    return m_impl->GetOrCreateTexture(device, resource);
}

void ImGuiDX12::Remove(ID3D12Resource *resource)
{
    m_impl->Remove(resource);
}

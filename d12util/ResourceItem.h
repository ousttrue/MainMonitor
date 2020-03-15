#pragma once
#include "Helper.h"
#include <memory>

namespace d12u
{
enum class UploadStates
{
    None,
    Enqueued,
    Uploaded,
};

/// ID3D12Resource helper
///
/// * State maintenance
/// * Barrier helper
///
class ResourceItem : public std::enable_shared_from_this<ResourceItem>
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_resource;

    struct ItemState
    {
        D3D12_RESOURCE_STATES State = {};
        UploadStates Upload = UploadStates::None;

        bool Drawable()
        {
            if (Upload != UploadStates::Uploaded)
            {
                return false;
            }

            switch (State)
            {
            case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
            case D3D12_RESOURCE_STATE_GENERIC_READ:
            case D3D12_RESOURCE_STATE_INDEX_BUFFER:
            case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
                return true;
            }

            return false;
        }
    };
    ItemState m_state = {};

    UINT m_byteLength = 0;
    UINT m_stride = 0;
    UINT m_count = 0;

    ResourceItem(const ComPtr<ID3D12Resource> &resource,
                 D3D12_RESOURCE_STATES state,
                 LPCWSTR name);

    // avoid copy
    ResourceItem(const ResourceItem &src) = delete;
    ResourceItem &operator=(const ResourceItem &src) = delete;

public:
    ItemState State() const { return m_state; }
    const ComPtr<ID3D12Resource> &Resource() const { return m_resource; }
    UINT Count() const { return m_count; }

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
    {
        return D3D12_VERTEX_BUFFER_VIEW{
            .BufferLocation = m_resource->GetGPUVirtualAddress(),
            .SizeInBytes = m_byteLength,
            .StrideInBytes = m_stride,
        };
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
    {
        auto view = D3D12_INDEX_BUFFER_VIEW{
            .BufferLocation = m_resource->GetGPUVirtualAddress(),
            .SizeInBytes = m_byteLength,
        };
        switch (m_stride)
        {
        case 4:
            view.Format = DXGI_FORMAT_R32_UINT;
            break;

        case 2:
            view.Format = DXGI_FORMAT_R16_UINT;
            break;

        default:
            throw;
        }
        return view;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const
    {
        return m_resource->GetGPUVirtualAddress();
    }

    void MapCopyUnmap(const void *p, UINT byteLength, UINT stride);
    void EnqueueTransition(class CommandList *commandList, D3D12_RESOURCE_STATES state);
    void EnqueueUpload(class CommandList *commandList, const std::shared_ptr<class ResourceItem> &upload,
                       const void *p, UINT byteLength, UINT stride);
    // dynamic
    static std::shared_ptr<ResourceItem> CreateUpload(const ComPtr<ID3D12Device> &device, UINT byteLength, LPCWSTR name);
    // static
    static std::shared_ptr<ResourceItem> CreateDefault(const ComPtr<ID3D12Device> &device, UINT byteLength, LPCWSTR name);
    // static image
    static std::shared_ptr<ResourceItem> CreateDefaultImage(const ComPtr<ID3D12Device> &device, UINT width, UINT height, LPCWSTR name);
};
} // namespace d12u
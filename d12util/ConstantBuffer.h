#pragma once
#include "Helper.h"
#include <stdint.h>
#include <vector>

namespace d12u
{

class ConstantBufferBase : NonCopyable
{
    //
    // CPU
    //
protected:
    std::vector<uint8_t> m_bytes;
    UINT8 *m_pCbvDataBegin = nullptr;

public:
    virtual UINT Count() const = 0;
    virtual std::pair<UINT, UINT> Range(UINT index) const = 0;

    //
    // GPU
    //
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

public:
    const Microsoft::WRL::ComPtr<ID3D12Resource> &Resource() const { return m_resource; }
    void CopyToGpu()
    {
        memcpy(m_pCbvDataBegin, m_bytes.data(), m_bytes.size());
    }
};

// class SemanticsConstantBuffer : public ConstantBufferBase
// {
//     UINT m_itemSize = 0;

// public:
// };

template <typename T>
class ConstantBuffer : public ConstantBufferBase
{
public:
    // CB size is required to be 256-byte aligned.
    static const UINT ITEM_SIZE = ((UINT)sizeof(T) + 255) & ~255;

    UINT Count() const override
    {
        return (UINT)(m_bytes.size() / ITEM_SIZE);
    }
    std::pair<UINT, UINT> Range(UINT index) const override
    {
        return std::make_pair(ITEM_SIZE * index, ITEM_SIZE);
    }

    T *GetTyped(int index)
    {
        return (T *)&m_bytes[ITEM_SIZE * index];
    }

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device, int count)
    {
        m_bytes.resize(ITEM_SIZE * count);

        D3D12_HEAP_PROPERTIES prop{
            .Type = D3D12_HEAP_TYPE_UPLOAD,
        };
        D3D12_RESOURCE_DESC desc{
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = (UINT)m_bytes.size(),
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {1, 0},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource)));

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        D3D12_RANGE readRange{0, 0}; // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void **>(&m_pCbvDataBegin)));
    }
};

} // namespace d12u

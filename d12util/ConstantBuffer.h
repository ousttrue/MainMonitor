#pragma once
#include "Helper.h"
#include <stdint.h>
#include <vector>

namespace d12u
{

class ConstantBufferBase : NonCopyable
{
    //
    // cpu
    //
protected:
    std::vector<uint8_t> m_bytes;
    UINT8 *m_pCbvDataBegin = nullptr;

public:
    virtual UINT Size() const = 0;
    UINT Count() const { return (UINT)(m_bytes.size() / Size()); }
    uint8_t *Get(int index)
    {
        return &m_bytes[Size() * index];
    }

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

template <typename T>
class ConstantBuffer : public ConstantBufferBase
{
    // // CB size is required to be 256-byte aligned.
    static const UINT SIZE = ((UINT)sizeof(T) + 255) & ~255;

public:
    UINT Size() const override { return SIZE; }
    T *GetTyped(int index)
    {
        return (T *)ConstantBufferBase::Get(index);
    }

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device, int count)
    {
        m_bytes.resize(SIZE * count);

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

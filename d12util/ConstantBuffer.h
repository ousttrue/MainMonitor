#pragma once
#include "Helper.h"
#include <stdint.h>

namespace d12u
{

class ConstantBufferBase : NonCopyable
{
public:
    virtual UINT Size() const = 0;
    virtual UINT Length() const = 0;
    virtual const Microsoft::WRL::ComPtr<ID3D12Resource> &Get() const = 0;
};

template <typename T, int length>
class ConstantBuffer : public ConstantBufferBase
{
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    UINT8 *m_pCbvDataBegin = nullptr;

    // CB size is required to be 256-byte aligned.
    static const UINT SIZE = ((UINT)sizeof(T) + 255) & ~255;
    static const UINT LENGTH = length;

    uint8_t m_bytes[SIZE * LENGTH] = {};

public:
    UINT Size() const override { return SIZE; }
    UINT Length() const override { return LENGTH; }
    const Microsoft::WRL::ComPtr<ID3D12Resource> &Get() const override { return m_resource; }

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device)
    {
        D3D12_HEAP_PROPERTIES prop{
            .Type = D3D12_HEAP_TYPE_UPLOAD,
        };
        D3D12_RESOURCE_DESC desc{
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = SIZE * length,
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

    void CopyToGpu()
    {
        memcpy(m_pCbvDataBegin, m_bytes, sizeof(m_bytes));
    }

    T *Get(int index)
    {
        return (T *)&m_bytes[SIZE * index];
    }
};
} // namespace d12u
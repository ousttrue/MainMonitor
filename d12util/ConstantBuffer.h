#pragma once
#include "Helper.h"
#include <stdint.h>

namespace d12u
{
template <typename T, int length, int start, int count, int step>
class ConstantBuffer
{
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    UINT8 *m_pCbvDataBegin = nullptr;

    // CB size is required to be 256-byte aligned.
    static const UINT SIZE = ((UINT)sizeof(T) + 255) & ~255;

    uint8_t m_bytes[SIZE * length] = {};

public:
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandles[count] = {};
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandles[count] = {};
    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &cbvHeap)
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

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = m_resource->GetGPUVirtualAddress(),
            .SizeInBytes = SIZE,
        };
        auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto cpuHandle = cbvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += (start * descriptorSize);
        auto gpuHandle = cbvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += (start * descriptorSize);
        for (int i = 0; i < count; ++i,
                 cpuHandle.ptr += descriptorSize * step,
                 gpuHandle.ptr += descriptorSize * step,
                 cbvDesc.BufferLocation += (length == count ? SIZE : 0))
        {
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);
            CpuHandles[i] = cpuHandle;
            GpuHandles[i] = gpuHandle;
        }

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
#include "Heap.h"
#include "ConstantBuffer.h"

namespace d12u
{

void Heap::Initialize(const ComPtr<ID3D12Device> &device,
                      UINT resourceCount, const ConstantBufferBase *const *resources)
{
    UINT count = 0;
    {
        for (UINT i = 0; i < resourceCount; ++i)
        {
            count += resources[i]->Count();
        }
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = count,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
    }

    auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto cpuHandle = m_cbvHeap->GetCPUDescriptorHandleForHeapStart();
    auto gpuHandle = m_cbvHeap->GetGPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < resourceCount; ++i)
    {
        auto resource = resources[i];
        for (UINT j = 0; j < resource->Count(); ++j,
                 cpuHandle.ptr += descriptorSize,
                 gpuHandle.ptr += descriptorSize)
        {
            auto size = resource->Size();
            auto offset = 0;
            if (resource->Count() > 1)
            {
                // each model cbv
                offset = size * j;
            }
            else
            {
                // shared scene cbv
            }
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
                .BufferLocation = resource->Resource()->GetGPUVirtualAddress() + offset,
                .SizeInBytes = size,
            };
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);
            m_cpuHandles.push_back(cpuHandle);
            m_gpuHandles.push_back(gpuHandle);
        }
    }
}

} // namespace d12u
#include "Heap.h"
#include "ConstantBuffer.h"

namespace d12u
{

void Heap::Initialize(const ComPtr<ID3D12Device> &device,
                      UINT resourceCount, const ConstantBufferBase *const *resources,
                      UINT srvSlots)
{
    UINT count = srvSlots;
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
        ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_heap)));
    }

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();

    auto index = 0;
    for (UINT i = 0; i < resourceCount; ++i)
    {
        auto resource = resources[i];
        for (UINT j = 0; j < resource->Count(); ++j, ++index)
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
            device->CreateConstantBufferView(&cbvDesc, CpuHandle(index));
        }
    }
}

} // namespace d12u
#include "Heap.h"
#include "ConstantBuffer.h"

namespace d12u
{

void Heap::Initialize(const ComPtr<ID3D12Device> &device,
                      ConstantBufferBase *const *resources, UINT resourceCount,
                      UINT groupCount)
{
    {
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = resourceCount * groupCount,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
    }

    auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto cpuHandle = m_cbvHeap->GetCPUDescriptorHandleForHeapStart();
    auto gpuHandle = m_cbvHeap->GetGPUDescriptorHandleForHeapStart();

    // cbv switch count
    for (UINT i = 0; i < groupCount; ++i)
    {
        // shader cbv count
        for (UINT j = 0; j < resourceCount; ++j,
                  cpuHandle.ptr += descriptorSize,
                  gpuHandle.ptr += descriptorSize)
        {
            auto resource = resources[j];
            auto size = resource->Size();
            auto offset = resource->Length() > 1
                              ? resource->Size() * i // each model cbv
                              : 0 // shared scene cbv
                ;
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
                .BufferLocation = resource->Get()->GetGPUVirtualAddress() + offset,
                .SizeInBytes = size,
            };
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);
            m_cpuHandles.push_back(cpuHandle);
            m_gpuHandles.push_back(gpuHandle);
        }
    }
}

} // namespace d12u
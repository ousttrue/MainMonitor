#include "Heap.h"
#include "ConstantBuffer.h"

namespace d12u
{

void Heap::Initialize(const ComPtr<ID3D12Device> &device, UINT count)
{
    {
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
}

} // namespace d12u
#pragma once
#include "Helper.h"
#include "ConstantBuffer.h"
#include <DirectXMath.h>
#include <memory>

namespace d12u
{
class Pipeline
{
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    class CommandList *m_commandList = nullptr;

public:
    Pipeline();
    ~Pipeline();
    const ComPtr<ID3D12DescriptorHeap> &Heap() const { return m_cbvHeap; }
    bool Initialize(const ComPtr<ID3D12Device> &device, const std::string &shaderSource);
    class CommandList *Reset();
};
} // namespace d12u
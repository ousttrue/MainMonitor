#pragma once
#include "Helper.h"
#include "ConstantBuffer.h"
#include <DirectXMath.h>
#include <memory>

namespace d12u
{
class Pipeline: NonCopyable
{
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    class CommandList *m_commandList = nullptr;

public:
    Pipeline();
    ~Pipeline();
    bool Initialize(const ComPtr<ID3D12Device> &device, const std::string &shaderSource, UINT cbvDescriptorCount);
    class CommandList *Reset();
};
} // namespace d12u
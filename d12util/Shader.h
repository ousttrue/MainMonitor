#pragma once
#include "Helper.h"

namespace d12u
{

class Shader : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12PipelineState> m_pipelineState;
    int m_generation = -1;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const std::string &source, int generation);
    bool Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u

#pragma once
#include "Helper.h"

namespace d12u
{

class Shader : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12PipelineState> m_pipelineState;

public:
    std::string m_vs;
    std::string m_ps;

    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature);
    void Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u

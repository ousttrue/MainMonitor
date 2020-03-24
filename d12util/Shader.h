#pragma once
#include "Helper.h"
#include <d3dcompiler.h>

namespace d12u
{

class Shader : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    int m_generation = -1;

    // keep semantics string
    std::vector<std::string> m_semantics;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
    bool InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

    std::string m_name;

public:
    ComPtr<ID3DBlob> m_vs;
    ComPtr<ID3DBlob> m_ps;

    Shader(const std::string &name)
        : m_name(name)
    {
    }

    const D3D12_INPUT_ELEMENT_DESC *inputLayout(int *count) const
    {
        *count = (int)m_layout.size();
        return m_layout.data();
    }
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const std::string &source, int generation);
};

} // namespace d12u

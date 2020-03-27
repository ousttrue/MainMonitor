#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <memory>
#include "ShaderConstantVariable.h"

namespace hierarchy
{

class Shader
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

private:
    std::string m_name;
    int m_generation = -1;

    // keep semantics string
    std::vector<std::string> m_semantics;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
    bool InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

public:
    struct ShaderWithConstants
    {
        ComPtr<ID3DBlob> Compiled;

    private:
        std::vector<ConstantBuffer> Buffers;

    public:
        const ConstantBuffer *DrawCB() const
        {
            for (auto &b : Buffers)
            {
                if (b.reg == 1)
                {
                    return &b;
                }
            }
            return nullptr;
        }

        D3D12_SHADER_BYTECODE ByteCode() const
        {
            return {
                Compiled->GetBufferPointer(),
                Compiled->GetBufferSize(),
            };
        }

        void ShaderWithConstants::GetConstants(const ComPtr<ID3D12ShaderReflection> &pReflection,
                                               const std::string &source);
    };

public:
    ShaderWithConstants VS;
    ShaderWithConstants PS;

    Shader(const std::string &name)
        : m_name(name)
    {
    }

    int Generation() const { return m_generation; }

    const D3D12_INPUT_ELEMENT_DESC *inputLayout(int *count) const
    {
        *count = (int)m_layout.size();
        return m_layout.data();
    }
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const std::string &source, int generation);
};
using ShaderPtr = std::shared_ptr<Shader>;

} // namespace hierarchy

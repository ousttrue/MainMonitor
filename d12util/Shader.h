#pragma once
#include "Helper.h"
#include <d3dcompiler.h>

namespace d12u
{

class Shader : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
    std::string m_name;
    int m_generation = -1;

    // keep semantics string
    std::vector<std::string> m_semantics;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
    bool InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

public:
    enum class ConstantSemantics
    {
        UNKNOWN,

        RENDERTARGET_SIZE,
        CAMERA_VIEW,
        CAMERA_PROJECTION,
        CAMERA_POSITION,
        CAMERA_FOVY,
        LIGHT_DIRECTION,
        LIGHT_COLOR,
        NODE_WORLD,
    };

    struct ConstantVariable
    {
        std::string Name;
        ConstantSemantics Semantic;
        UINT Offset;
        UINT Size;

        void GetSemantic(const std::string &src);
    };

    struct ConstantBuffer
    {
        std::vector<ConstantVariable> Variables;

        void GetVariables(ID3D12ShaderReflectionConstantBuffer *cb,
                          const std::string &source);
    };

    struct ShaderWithConstants
    {
        ComPtr<ID3DBlob> Compiled;

        std::vector<ConstantBuffer> Buffers;

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

} // namespace d12u

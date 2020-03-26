#include "Shader.h"
#include <plog/log.h>

namespace d12u
{

static std::string ToString(const Microsoft::WRL::ComPtr<ID3DBlob> &blob)
{
    std::vector<uint8_t> buffer(blob->GetBufferSize());
    memcpy(buffer.data(), blob->GetBufferPointer(), buffer.size());
    return std::string(buffer.begin(), buffer.end());
}

static bool IsMatch(const std::string &src, const std::string &name, Shader::ConstantSemantics semantic)
{
    auto pos = src.find(name);
    if (pos == std::string::npos)
    {
        // not found
        return false;
    }

    auto tail = src[pos + name.size()];
    if (
        (tail >= 'a' && tail <= 'z')    // a-z
        || (tail >= 'A' && tail <= 'Z') // A-Z
        || (tail >= '0' && tail <= '9') // 0-9
        || tail == '_')
    {
        // continuous
        return false;
    }

    return true;
}

#define MATCH(Symbol)                                             \
    if (IsMatch(src, #Symbol, Shader::ConstantSemantics::Symbol)) \
    {                                                             \
        return Shader::ConstantSemantics::Symbol;                 \
    }

static Shader::ConstantSemantics GetSemanticAfterColon(const std::string &src)
{
    MATCH(RENDERTARGET_SIZE);
    MATCH(CAMERA_VIEW);
    MATCH(CAMERA_PROJECTION);
    MATCH(CAMERA_POSITION);
    MATCH(CAMERA_FOVY);
    MATCH(LIGHT_DIRECTION);
    MATCH(LIGHT_COLOR);
    MATCH(NODE_WORLD);

    return Shader::ConstantSemantics::UNKNOWN;
}

#undef MATCH

static Shader::ConstantSemantics GetSemanticAfterName(const std::string &src)
{
    // search :
    for (auto it = src.begin(); it != src.end(); ++it)
    {
        if (*it == ';')
        {
            return Shader::ConstantSemantics::UNKNOWN;
        }

        if (*it == ':')
        {
            auto start = it;
            ++start;
            for (; start != src.end() && *start == ' '; ++start)
            {
            }

            auto end = start;
            ++end;
            for (; end != src.end() && *end != ';'; ++end)
            {
            }

            return GetSemanticAfterColon(std::string(start, end));
        }
    }

    return Shader::ConstantSemantics::UNKNOWN;
}

void Shader::ConstantVariable::GetSemantic(const std::string &src)
{
    auto found = src.find(Name);
    if (found == std::string::npos)
    {
        return;
    }

    Semantic = GetSemanticAfterName(src.substr(found + Name.size()));
}

void Shader::ConstantBuffer::GetVariables(ID3D12ShaderReflectionConstantBuffer *cb,
                                          const std::string &source)
{
    D3D12_SHADER_BUFFER_DESC cbDesc;
    cb->GetDesc(&cbDesc);
    for (unsigned j = 0; j < cbDesc.Variables; ++j)
    {
        auto cbVariable = cb->GetVariableByIndex(j);
        D3D12_SHADER_VARIABLE_DESC variableDesc;
        cbVariable->GetDesc(&variableDesc);
        Variables.push_back(ConstantVariable{
            .Name = variableDesc.Name,
            .Offset = variableDesc.StartOffset,
            .Size = variableDesc.Size,
        });
        Variables.back().GetSemantic(source);
    }
}

void Shader::ShaderWithConstants::GetConstants(const ComPtr<ID3D12ShaderReflection> &pReflection,
                                               const std::string &source)
{
    D3D12_SHADER_DESC desc;
    pReflection->GetDesc(&desc);

    for (unsigned i = 0; i < desc.ConstantBuffers; ++i)
    {
        auto cb = pReflection->GetConstantBufferByIndex(i);
        Buffers.push_back({});
        Buffers.back().GetVariables(cb, source);
    }
}

bool Shader::InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &pReflection)
{
    // Define the vertex input layout.
    // D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    //     {
    //         {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //         {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //         {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //     };
    // auto inputElementDescs = InputLayoutFromReflection(vertexShader);

    D3D12_SHADER_DESC desc;
    pReflection->GetDesc(&desc);
    m_semantics.reserve(desc.InputParameters);
    for (unsigned i = 0; i < desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC lParamDesc;
        pReflection->GetInputParameterDesc(i, &lParamDesc);

        m_semantics.push_back(lParamDesc.SemanticName);
        D3D12_INPUT_ELEMENT_DESC lElementDesc{
            .SemanticName = m_semantics.back().c_str(),
            .SemanticIndex = lParamDesc.SemanticIndex,
            .InputSlot = 0,
            .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        if (lParamDesc.Mask == 1)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (lParamDesc.Mask <= 3)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (lParamDesc.Mask <= 7)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (lParamDesc.Mask <= 15)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else
        {
            throw "unknown";
        }

        m_layout.push_back(lElementDesc);
    }

    return true;
}

bool Shader::Initialize(const ComPtr<ID3D12Device> &device,
                        const std::string &source,
                        int generation)
{
    if (generation > m_generation)
    {
        // clear
        VS.Compiled = nullptr;
        PS.Compiled = nullptr;
        m_semantics.clear();
        m_layout.clear();
    }

    if (VS.Compiled && PS.Compiled)
    {
        // already
        return true;
    }

    if (source.empty())
    {
        return false;
    }

    // Create the pipeline state, which includes compiling and loading shaders.
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    //
    // VS
    //
    {
        ComPtr<ID3DBlob> error;
        if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &VS.Compiled, &error)))
        {
            LOGW << ToString(error);
            return false;
        }
        ComPtr<ID3D12ShaderReflection> pReflection;
        if (FAILED(D3DReflect(VS.Compiled->GetBufferPointer(), VS.Compiled->GetBufferSize(), IID_PPV_ARGS(&pReflection))))
        {
            return false;
        }
        if (!InputLayoutFromReflection(pReflection))
        {
            return false;
        }
        VS.GetConstants(pReflection, source);
    }

    //
    // PS
    //
    {
        ComPtr<ID3DBlob> error;
        if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &PS.Compiled, nullptr)))
        {
            LOGW << ToString(error);
            return false;
        }
        ComPtr<ID3D12ShaderReflection> pReflection;
        if (FAILED(D3DReflect(PS.Compiled->GetBufferPointer(), PS.Compiled->GetBufferSize(), IID_PPV_ARGS(&pReflection))))
        {
            return false;
        }
        PS.GetConstants(pReflection, source);
    }

    m_generation = generation;
    return true;
}

} // namespace d12u

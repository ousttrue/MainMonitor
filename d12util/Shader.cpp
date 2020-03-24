#include "Shader.h"
#include <plog/log.h>

namespace d12u
{

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
                        const ComPtr<ID3D12RootSignature> &rootSignature,
                        const std::string &source,
                        int generation)
{
    if (m_pipelineState)
    {
        if (generation > m_generation)
        {
            // clear
            m_pipelineState = nullptr;
            m_semantics.clear();
            m_layout.clear();
        }
        else
        {
            // already
            return true;
        }
    }

    if (source.empty())
    {
        return false;
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        {
            ComPtr<ID3DBlob> error;
            if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
            {
                LOGW << ToString(error);
                return false;
            }
        }
        {
            ComPtr<ID3DBlob> error;
            if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr)))
            {
                LOGW << ToString(error);
                return false;
            }
        }

        ComPtr<ID3D12ShaderReflection> pReflection;
        if (FAILED(D3DReflect(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(&pReflection))))
        {
            return false;
        }
        if (!InputLayoutFromReflection(pReflection))
        {
            return false;
        }

        {
            D3D12_SHADER_DESC desc;
            pReflection->GetDesc(&desc);

            for (unsigned i = 0; i < desc.ConstantBuffers; ++i)
            {
                auto cb = pReflection->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC cbDesc;
                cb->GetDesc(&cbDesc);
                for (unsigned j = 0; j < cbDesc.Variables; ++j)
                {
                    auto cbVariable = cb->GetVariableByIndex(j);
                    D3D12_SHADER_VARIABLE_DESC variableDesc;
                    cbVariable->GetDesc(&variableDesc);
                    auto a = 0;
                }
            }
        }

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = rootSignature.Get(),
            .VS = {
                .pShaderBytecode = vertexShader->GetBufferPointer(),
                .BytecodeLength = vertexShader->GetBufferSize(),
            },
            .PS = {
                .pShaderBytecode = pixelShader->GetBufferPointer(),
                .BytecodeLength = pixelShader->GetBufferSize(),
            },
            .BlendState = {
                .AlphaToCoverageEnable = FALSE,
                .IndependentBlendEnable = FALSE,
                .RenderTarget = {
                    {
                        FALSE,
                        FALSE,
                        D3D12_BLEND_ONE,
                        D3D12_BLEND_ZERO,
                        D3D12_BLEND_OP_ADD,
                        D3D12_BLEND_ONE,
                        D3D12_BLEND_ZERO,
                        D3D12_BLEND_OP_ADD,
                        D3D12_LOGIC_OP_NOOP,
                        D3D12_COLOR_WRITE_ENABLE_ALL,
                    },
                },
            },
            .SampleMask = UINT_MAX,
            .RasterizerState = {
                .FillMode = D3D12_FILL_MODE_SOLID,
                .CullMode = D3D12_CULL_MODE_BACK,
                // .CullMode = D3D12_CULL_MODE_NONE,
                .FrontCounterClockwise = TRUE,
                .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
                .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
                .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
                .DepthClipEnable = TRUE,
                .MultisampleEnable = FALSE,
                .AntialiasedLineEnable = FALSE,
                .ForcedSampleCount = 0,
                .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
            },
            .DepthStencilState = {
                .DepthEnable = TRUE,
                .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
                .DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
                .StencilEnable = FALSE,
                .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
                .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
                .FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS},
                .BackFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS},
            },
            .InputLayout = {m_layout.data(), (UINT)m_layout.size()},
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
            .DSVFormat = DXGI_FORMAT_D32_FLOAT,
            .SampleDesc{
                .Count = 1,
            },
        };

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        // m_commandList->Initialize(device, m_pipelineState);
    }

    m_generation = generation;
    return true;
}

bool Shader::Set(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    if (!m_pipelineState)
    {
        return false;
    }
    commandList->SetPipelineState(m_pipelineState.Get());
    return true;
}

} // namespace d12u

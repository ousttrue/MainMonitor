#include "Material.h"
#include <d3dcompiler.h>

namespace d12u
{

bool Material::Initialize(const ComPtr<ID3D12Device> &device,
                          const ComPtr<ID3D12RootSignature> &rootSignature,
                          const std::string &shaderSource, UINT cbvDescriptorCount)
{
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
            if (FAILED(D3DCompile(shaderSource.data(), shaderSource.size(), "shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
            {
                PrintBlob(error);
                throw;
            }
        }
        {
            ComPtr<ID3DBlob> error;
            if (FAILED(D3DCompile(shaderSource.data(), shaderSource.size(), "shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr)))
            {
                PrintBlob(error);
                throw;
            }
        }

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            };

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
                .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
                .StencilEnable = FALSE,
                .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
                .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
                .FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS},
                .BackFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS},
            },
            .InputLayout = {inputElementDescs, _countof(inputElementDescs)},
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

    return true;
}

void Material::Set(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
}

} // namespace d12u

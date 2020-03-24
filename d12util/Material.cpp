#include "Material.h"
#include "Shader.h"

namespace d12u
{

bool Material::Initialize(const ComPtr<ID3D12Device> &device,
                          const ComPtr<ID3D12RootSignature> &rootSignature,
                          const std::shared_ptr<Shader> &shader,
                          const hierarchy::SceneMaterialPtr &material)
{
    int inputLayoutCount;
    auto inputLayout = shader->inputLayout(&inputLayoutCount);

    D3D12_BLEND_DESC blend{
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget = {
            {
                .BlendEnable = FALSE,
                .LogicOpEnable = FALSE,
                .SrcBlend = D3D12_BLEND_ONE,
                .DestBlend = D3D12_BLEND_ZERO,
                .BlendOp = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D12_BLEND_ONE,
                .DestBlendAlpha = D3D12_BLEND_ZERO,
                .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                .LogicOp = D3D12_LOGIC_OP_NOOP,
                .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
            },
        },
    };

    switch (material->alphaMode)
    {
    case hierarchy::AlphaMode::Opaque:
        break;

    case hierarchy::AlphaMode::Mask:
        break;

    case hierarchy::AlphaMode::Blend:
        blend.RenderTarget[0] = {
            .BlendEnable = TRUE,
            .LogicOpEnable = FALSE,
            .SrcBlend = D3D12_BLEND_SRC_ALPHA,
            .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D12_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA,
            .DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
            .BlendOpAlpha = D3D12_BLEND_OP_ADD,
            .LogicOp = D3D12_LOGIC_OP_NOOP,
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        break;

    default:
        throw;
    }

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = rootSignature.Get(),
        .VS = {
            .pShaderBytecode = shader->m_vs->GetBufferPointer(),
            .BytecodeLength = shader->m_vs->GetBufferSize(),
        },
        .PS = {
            .pShaderBytecode = shader->m_ps->GetBufferPointer(),
            .BytecodeLength = shader->m_ps->GetBufferSize(),
        },
        .BlendState = blend,
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
        .InputLayout = {inputLayout, (UINT)inputLayoutCount},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc{
            .Count = 1,
        },
    };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    return true;
}

bool Material::Set(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    if (!m_pipelineState)
    {
        return false;
    }
    commandList->SetPipelineState(m_pipelineState.Get());
    return true;
}

} // namespace d12u
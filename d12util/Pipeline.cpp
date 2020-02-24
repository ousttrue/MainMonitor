#include "Pipeline.h"
#include "SwapChain.h"
#include "ResourceItem.h"
#include "CommandList.h"
#include "Mesh.h"
#include <string>
#include <d3dcompiler.h>
#include <algorithm>
#include <iostream>

namespace d12u
{
using namespace DirectX;

Pipeline::Pipeline()
    : m_commandList(new CommandList)
{
}

Pipeline::~Pipeline()
{
    delete m_commandList;
}

static void PrintBlob(const ComPtr<ID3DBlob> &blob)
{
    std::vector<uint8_t> buffer(blob->GetBufferSize());
    memcpy(buffer.data(), blob->GetBufferPointer(), buffer.size());
    std::string msg(buffer.begin(), buffer.end());
    std::cerr << msg << std::endl;
}

bool Pipeline::Initialize(const ComPtr<ID3D12Device> &device, const std::string &shaderSource, UINT cbvDescriptorCount)
{

    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {
            // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
            .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1,
        };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            throw;
        }

        D3D12_DESCRIPTOR_RANGE1 ranges[] = {
            {
                .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                .NumDescriptors = 1,
                .BaseShaderRegister = 0,
                .RegisterSpace = 0,
                .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                // . OffsetInDescriptorsFromTableStart,
            },
            {
                .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                .NumDescriptors = 1,
                .BaseShaderRegister = 1,
                .RegisterSpace = 0,
                .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                // . OffsetInDescriptorsFromTableStart,
            },
        };
        D3D12_ROOT_PARAMETER1 rootParameters[] = {
            // scene
            {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                .DescriptorTable = {
                    .NumDescriptorRanges = 1,
                    .pDescriptorRanges = &ranges[0],
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            // world
            {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                .DescriptorTable = {
                    .NumDescriptorRanges = 1,
                    .pDescriptorRanges = &ranges[1],
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
        };

        // Allow input layout and deny unecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT //
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS     //
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS   //
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS //
            // | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS      //
            ;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{
            .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
            .Desc_1_1{
                .NumParameters = _countof(rootParameters),
                .pParameters = rootParameters,
                .Flags = rootSignatureFlags,
            },
        };

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
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
            .pRootSignature = m_rootSignature.Get(),
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
        m_commandList->Initialize(device, m_pipelineState);
    }

    return true;
}

CommandList *Pipeline::Reset()
{
    m_commandList->Reset(m_pipelineState);

    auto commandList = m_commandList->Get();

    // Set necessary state.
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    return m_commandList;
}
} // namespace d12u
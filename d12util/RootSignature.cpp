#include "RootSignature.h"
#include "Material.h"
#include <d3dcompiler.h>
#include <algorithm>

namespace d12u
{

RootSignature::RootSignature()
    : m_heap(new Heap)
{
}

bool RootSignature::Initialize(const ComPtr<ID3D12Device> &device)
{
    // Create a root signature consisting of a descriptor table with a single CBV.
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

    //
    // buffers
    //
    m_sceneConstantsBuffer.Initialize(device, 1);
    m_nodeConstantsBuffer.Initialize(device, 128);
    m_materialConstantsBuffer.Initialize(device, 128);
    ConstantBufferBase *items[] = {
        &m_sceneConstantsBuffer,
        &m_nodeConstantsBuffer,
        &m_nodeConstantsBuffer,
    };
    m_heap->Initialize(device, _countof(items), items);

    return true;
}

void RootSignature::Begin(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap *ppHeaps[] = {m_heap->Get()};
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    commandList->SetGraphicsRootDescriptorTable(0, m_heap->GpuHandle(0));
}

std::shared_ptr<Material> RootSignature::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMaterial> &sceneMaterial)
{
    auto found = m_materialMap.find(sceneMaterial);
    if (found != m_materialMap.end())
    {
        return found->second;
    }

    auto gpuMaterial = std::make_shared<Material>();

    // shader
    if (!gpuMaterial->Initialize(device, m_rootSignature, sceneMaterial->shader, 2))
    {
        return nullptr;
    }

    m_materialMap.insert(std::make_pair(sceneMaterial, gpuMaterial));
    return gpuMaterial;
}

} // namespace d12u
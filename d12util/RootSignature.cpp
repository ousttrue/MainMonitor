#include "RootSignature.h"
#include "Shader.h"
#include "Material.h"
#include "ResourceItem.h"
#include "Texture.h"
#include "Uploader.h"
#include <d3dcompiler.h>
#include <algorithm>

// SCENE_SLOTS=1;
const int NODE_SLOTS = 1024;
const int MATERIAL_SLOTS = 1024;
const int TEXTURE_SLOTS = MATERIAL_SLOTS;

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
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            // .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
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
        // SRV
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[2],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
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

    D3D12_STATIC_SAMPLER_DESC sampler = {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
    rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    //
    // buffers
    //
    m_sceneConstantsBuffer.Initialize(device, 1);
    m_nodeConstantsBuffer.Initialize(device, NODE_SLOTS);
    m_materialConstantsBuffer.Initialize(device, MATERIAL_SLOTS);
    ConstantBufferBase *items[] = {
        &m_sceneConstantsBuffer,
        &m_nodeConstantsBuffer,
        &m_materialConstantsBuffer,
    };
    m_heap->Initialize(device, _countof(items), items, TEXTURE_SLOTS);

    return true;
}

void RootSignature::Update(const ComPtr<ID3D12Device> &device)
{
    // update shader
    for (auto kv : m_shaderMap)
    {
        auto [source, generation] = kv.first->source();
        if (!source.empty())
        {
            if (!kv.second->Initialize(device, source, generation))
            {
                kv.first->clear();
            }
        }
    }

    for (auto kv : m_materialMap)
    {
        kv.second->Initialize(device, kv.first);
    }
}

void RootSignature::Begin(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap *ppHeaps[] = {m_heap->Get()};
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    commandList->SetGraphicsRootDescriptorTable(0, m_heap->GpuHandle(0));
}

std::shared_ptr<Shader> RootSignature::GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::ShaderWatcherPtr &shader)
{
    auto found = m_shaderMap.find(shader);
    if (found != m_shaderMap.end())
    {
        return found->second;
    }

    auto gpuShader = std::make_shared<Shader>(shader->name());
    auto [source, generation] = shader->source();
    if (!source.empty())
    {
        if (!gpuShader->Initialize(device, source, generation))
        {
            shader->clear();
        }
    }

    m_shaderMap.insert(std::make_pair(shader, gpuShader));
    return gpuShader;
}

std::shared_ptr<Material> RootSignature::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMaterial> &sceneMaterial)
{
    auto found = m_materialMap.find(sceneMaterial);
    if (found != m_materialMap.end())
    {
        return found->second;
    }

    // shader
    auto gpuShader = GetOrCreate(device, sceneMaterial->shader);
    if (!gpuShader)
    {
        return nullptr;
    }

    auto gpuMaterial = std::make_shared<Material>();
    if (!gpuMaterial->Initialize(device, m_rootSignature, gpuShader, sceneMaterial))
    {
        throw;
    }

    m_materialMap.insert(std::make_pair(sceneMaterial, gpuMaterial));
    return gpuMaterial;
}

std::pair<std::shared_ptr<class Texture>, UINT> RootSignature::GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneImagePtr &image,
                                                                           Uploader *uploader)
{
    auto found = m_textureMap.find(image);
    if (found != m_textureMap.end())
    {
        return std::make_pair(m_textures[found->second], found->second);
    }

    // create texture
    auto gpuTexture = std::make_shared<Texture>();
    {
        auto resource = ResourceItem::CreateDefaultImage(device, image->width, image->height, image->name.c_str());
        gpuTexture->ImageBuffer(resource);
        uploader->EnqueueUpload(resource, image->buffer.data(), (UINT)image->buffer.size(), image->width * 4);
    }

    auto index = (UINT)m_textures.size();
    m_textures.push_back(gpuTexture);
    m_textureMap.insert(std::make_pair(image, index));

    // create view
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1,
        },
    };
    device->CreateShaderResourceView(gpuTexture->Resource().Get(), &desc, m_heap->CpuHandle(index + 1 + NODE_SLOTS + MATERIAL_SLOTS));

    return std::make_pair(gpuTexture, index);
}

void RootSignature::SetNodeDescriptorTable(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex)
{
    commandList->SetGraphicsRootDescriptorTable(1, m_heap->GpuHandle(nodeIndex + 1));
}

void RootSignature::SetTextureDescriptorTable(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT textureIndex)
{
    commandList->SetGraphicsRootDescriptorTable(2, m_heap->GpuHandle(textureIndex + 1 + NODE_SLOTS + MATERIAL_SLOTS));
}

} // namespace d12u
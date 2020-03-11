#pragma once
#include "Helper.h"
#include "Heap.h"
#include "ConstantBuffer.h"
#include <memory>
#include <array>
#include <unordered_map>
#include <SceneMaterial.h>
#include <DirectXMath.h>

namespace d12u
{

/// Shader spec
///
/// * each ConstantBuffer type
/// * manipulation of HeapDescriptor
///
class RootSignature : NonCopyable
{
    ComPtr<ID3D12RootSignature> m_rootSignature;
    std::unique_ptr<Heap> m_heap;

    // scene
    struct SceneConstants
    {
        DirectX::XMFLOAT4X4 b0View;
        DirectX::XMFLOAT4X4 b0Projection;
        DirectX::XMFLOAT3 b0LightDir;
        DirectX::XMFLOAT3 b0LightColor;
    };
    d12u::ConstantBuffer<SceneConstants> m_sceneConstantsBuffer;

    // node
    struct NodeConstants
    {
        std::array<float, 16> b1World{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    };
    d12u::ConstantBuffer<NodeConstants> m_nodeConstantsBuffer;

    // material
    struct MaterialConstants
    {
        DirectX::XMFLOAT4 b1Diffuse;
        DirectX::XMFLOAT3 b1Ambient;
        DirectX::XMFLOAT3 b1Specular;
    };
    d12u::ConstantBuffer<MaterialConstants> m_materialConstantsBuffer;

    std::unordered_map<hierarchy::SceneMaterialPtr, std::shared_ptr<class Material>> m_materialMap;

public:
    RootSignature();
    bool Initialize(const ComPtr<ID3D12Device> &device);
    void Begin(const ComPtr<ID3D12GraphicsCommandList> &commandList);
    std::shared_ptr<class Material> GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMaterial> &material);

    SceneConstants *GetSceneConstantsBuffer(UINT index)
    {
        return m_sceneConstantsBuffer.Get(index);
    }
    void UploadSceneConstantsBuffer()
    {
        m_sceneConstantsBuffer.CopyToGpu();
    }
    NodeConstants *GetNodeConstantsBuffer(UINT index)
    {
        return m_nodeConstantsBuffer.Get(index);
    }
    void UploadNodeConstantsBuffer()
    {
        m_nodeConstantsBuffer.CopyToGpu();
    }
    void SetNodeDescriptorTable(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex)
    {
        // node constant
        // index[0] => camera
        // index[1-64] => world matrix
        commandList->SetGraphicsRootDescriptorTable(1, m_heap->GpuHandle(nodeIndex + 1));
    }
};

} // namespace d12u

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

    // Frame
    struct FrameConstants
    {
        DirectX::XMFLOAT4X4 b0View;
        DirectX::XMFLOAT4X4 b0Projection;
        DirectX::XMFLOAT3 b0LightDir;
        float p0;
        DirectX::XMFLOAT3 b0LightColor;
        float p1;
        DirectX::XMFLOAT3 b0CameraPosition;
        float p2;
        DirectX::XMFLOAT3 b0ScreenSizeFovY;
        // DirectX::XMFLOAT4X4 b0ViewInv;
    };
    d12u::ConstantBuffer<FrameConstants> m_sceneConstantsBuffer;

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

    std::unordered_map<hierarchy::ShaderWatcherPtr, std::shared_ptr<class Shader>> m_shaderMap;
    std::unordered_map<hierarchy::SceneMaterialPtr, std::shared_ptr<class Material>> m_materialMap;
    std::vector<std::shared_ptr<class Texture>> m_textures;
    std::unordered_map<hierarchy::SceneImagePtr, uint32_t> m_textureMap;

public:
    RootSignature();
    bool Initialize(const ComPtr<ID3D12Device> &device);
    // polling shader update
    void Update(const ComPtr<ID3D12Device> &device);
    void Begin(const ComPtr<ID3D12GraphicsCommandList> &commandList);
    std::shared_ptr<class Shader> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::ShaderWatcherPtr &shader);
    std::shared_ptr<class Material> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneMaterialPtr &material);
    std::pair<std::shared_ptr<class Texture>, UINT> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneImagePtr &image, class Uploader *uploader);

    FrameConstants *GetSceneConstantsBuffer(UINT index)
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
    void SetNodeDescriptorTable(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex);
    void SetTextureDescriptorTable(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT textureIndex);
};

} // namespace d12u

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

    // std::unordered_map<hierarchy::ShaderWatcherPtr, std::shared_ptr<class Shader>> m_shaderMap;
    std::unordered_map<hierarchy::SceneMaterialPtr, std::shared_ptr<class Material>> m_materialMap;
    std::vector<std::shared_ptr<class Texture>> m_textures;
    std::unordered_map<hierarchy::SceneImagePtr, uint32_t> m_textureMap;

public:
    RootSignature();
    bool Initialize(const ComPtr<ID3D12Device> &device);
    // polling shader update
    void Update(const ComPtr<ID3D12Device> &device);
    void Begin(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12GraphicsCommandList> &commandList);
    // std::shared_ptr<class Shader> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::ShaderWatcherPtr &shader);
    std::shared_ptr<class Material> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneMaterialPtr &material);
    std::pair<std::shared_ptr<class Texture>, UINT> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneImagePtr &image, class Uploader *uploader);

    // each View
    struct ViewConstants
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
    d12u::ConstantBuffer<ViewConstants> m_viewConstantsBuffer;
    ViewConstants *GetViewConstantsBuffer(UINT index)
    {
        return m_viewConstantsBuffer.GetTyped(index);
    }

    // each DrawCall
    d12u::SemanticsConstantBuffer m_drawConstantsBuffer{1024};

    void SetDrawDescriptorTable(const ComPtr<ID3D12Device> &device,
                                const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex);
    void SetTextureDescriptorTable(const ComPtr<ID3D12Device> &device,
                                   const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT textureIndex);
};

} // namespace d12u

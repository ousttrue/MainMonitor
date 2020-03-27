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
    std::vector<std::pair<uint32_t, uint32_t>> m_viewList;

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
// https://gamedev.stackexchange.com/questions/105572/c-struct-doesnt-align-correctly-to-a-pixel-shader-cbuffer
#pragma pack(push)
#pragma pack(16)
    struct ViewConstants
    {
        std::array<float, 16> b0View;
        std::array<float, 16> b0Projection;
        std::array<float, 3> b0LightDir;
        float _padding0;
        std::array<float, 3> b0LightColor;
        float _padding1;
        std::array<float, 3> b0CameraPosition;
        float _padding2;
        std::array<float, 2> b0ScreenSize;
        float fovY;
        float _padding3;
        // DirectX::XMFLOAT4X4 b0ViewInv;
    };
#pragma pack(pop)
    static_assert(sizeof(ViewConstants) == 16 * 12, "sizeof ViewConstantsSize");

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

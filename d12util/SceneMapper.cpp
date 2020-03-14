#include "SceneMapper.h"
#include "ResourceItem.h"
#include "Mesh.h"
#include "Texture.h"
#include "Uploader.h"
#include "RootSignature.h"
#include "Shader.h"
#include <DirectXMath.h>

namespace d12u
{
SceneMapper::SceneMapper()
    : m_uploader(new Uploader)
{
}

void SceneMapper::Initialize(const ComPtr<ID3D12Device> &device)
{
    m_uploader->Initialize(device);
}

void SceneMapper::Update(const ComPtr<ID3D12Device> &device)
{
    m_uploader->Update(device);
}

static int GetStride(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return 4;

    case DXGI_FORMAT_R32G32_FLOAT:
        return 8;

    case DXGI_FORMAT_R32G32B32_FLOAT:
        return 12;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    }

    throw;
}

static std::shared_ptr<ResourceItem> CreateResourceItem(
    const ComPtr<ID3D12Device> &device,
    const std::unique_ptr<Uploader> &uploader,
    const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh,
    const D3D12_INPUT_ELEMENT_DESC *inputLayout,
    int inputLayoutCount)
{
    auto dstStride = 0;
    for (int i = 0; i < inputLayoutCount; ++i)
    {
        dstStride += GetStride(inputLayout[i].Format);
    }

    // interleaved
    auto interleaved = sceneMesh->GetVertices(hierarchy::Semantics::Interleaved);
    if (interleaved)
    {
        if (interleaved->stride != dstStride)
        {
            throw;
        }

        if (interleaved->isDynamic)
        {
            auto resource = ResourceItem::CreateUpload(device, (UINT)interleaved->buffer.size());
            // not enqueue
            return resource;
        }
        else
        {
            auto resource = ResourceItem::CreateDefault(device, (UINT)interleaved->buffer.size());
            uploader->EnqueueUpload(resource, interleaved->buffer.data(), (UINT)interleaved->buffer.size(), interleaved->stride);
            return resource;
        }
    }

    // planar
    auto command = std::make_shared<UploadCommand>();

    static const std::string POSITION = "POSITION";
    static const std::string NORMAL = "NORMAL";
    static const std::string TEXCOORD = "TEXCOORD";
    static const std::string COLOR = "COLOR";
    int offset = 0;
    for (int i = 0; i < inputLayoutCount; ++i)
    {
        auto input = inputLayout[i];
        const hierarchy::VertexBuffer *buffer = nullptr;
        if (input.SemanticName == POSITION)
        {
            buffer = sceneMesh->GetVertices(hierarchy::Semantics::Position);
        }
        else if (input.SemanticName == NORMAL)
        {
            buffer = sceneMesh->GetVertices(hierarchy::Semantics::Normal);
        }
        else if (input.SemanticName == TEXCOORD)
        {
            buffer = sceneMesh->GetVertices(hierarchy::Semantics::TexCoord);
        }
        else
        {
            throw;
        }

        if (buffer)
        {
            auto count = buffer->Count();
            if (i == 0)
            {
                // alloc buffer
                auto size = dstStride * count;
                command->Payload.resize(size);
            }
            auto src = buffer->buffer.data();
            auto p = command->Payload.data() + offset;
            for (UINT i = 0; i < count; ++i,
                      src += buffer->stride,
                      p += dstStride)
            {
                memcpy(p, src, buffer->stride);
                // *(DirectX::XMFLOAT2 *)p = *(DirectX::XMFLOAT2 *)src;
            }
        }

        offset += GetStride(inputLayout[i].Format);
    }

    auto resource = ResourceItem::CreateDefault(device, (UINT)command->Payload.size());
    command->UsePayload(resource, dstStride);
    // uploader->EnqueueUpload(command);
    uploader->EnqueueUpload(command);
    return resource;
}

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device,
                                               const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh,
                                               RootSignature *rootSignature)
{
    auto found = m_meshMap.find(sceneMesh);
    if (found != m_meshMap.end())
    {
        return found->second;
    }

    if (sceneMesh->submeshes.empty())
    {
        return nullptr;
    }

    auto gpuMesh = std::make_shared<Mesh>();

    // vertices
    {
        // first material's shader for input layout
        auto shader = rootSignature->GetOrCreate(device, sceneMesh->submeshes[0].material->shader);
        auto resource = CreateResourceItem(device, m_uploader, sceneMesh, shader->inputLayout(), shader->inputLayoutCount());
        if (!resource)
        {
            // fail
            return nullptr;
        }
        gpuMesh->VertexBuffer(resource);
    }

    // indices
    auto indices = sceneMesh->GetIndices();
    if (indices)
    {
        if (indices->isDynamic)
        {
            auto resource = ResourceItem::CreateUpload(device, (UINT)indices->buffer.size());
            gpuMesh->IndexBuffer(resource);
            // not enqueue
        }
        else
        {
            auto resource = ResourceItem::CreateDefault(device, (UINT)indices->buffer.size());
            gpuMesh->IndexBuffer(resource);
            m_uploader->EnqueueUpload(resource, indices->buffer.data(), (UINT)indices->buffer.size(), indices->stride);
        }
    }

    m_meshMap.insert(std::make_pair(sceneMesh, gpuMesh));
    return gpuMesh;
}

} // namespace d12u

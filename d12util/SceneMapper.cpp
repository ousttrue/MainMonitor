#include "SceneMapper.h"
#include "ResourceItem.h"
#include "Mesh.h"
#include "Texture.h"
#include "Uploader.h"
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

static std::shared_ptr<ResourceItem> CreateResourceItem(
    const ComPtr<ID3D12Device> &device,
    const std::unique_ptr<Uploader> &uploader,
    const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh)
{
    // interleaved
    auto positions = sceneMesh->GetVertices(hierarchy::Semantics::PositionNormalTexCoord);
    if (!positions)
    {
        positions = sceneMesh->GetVertices(hierarchy::Semantics::PositionNormalColor);
    }
    if (positions)
    {
        if (positions->isDynamic)
        {
            auto resource = ResourceItem::CreateUpload(device, (UINT)positions->buffer.size());
            // not enqueue
            return resource;
        }
        else
        {
            auto resource = ResourceItem::CreateDefault(device, (UINT)positions->buffer.size());
            uploader->EnqueueUpload(resource, positions->buffer.data(), (UINT)positions->buffer.size(), positions->Stride());
            return resource;
        }
    }

    // planar
    positions = sceneMesh->GetVertices(hierarchy::Semantics::Position);
    if (positions)
    {
        auto command = std::make_shared<UploadCommand>();
        uploader->EnqueueUpload(command);

        // alloc buffer
        auto count = positions->Count();
        auto stride = (int)hierarchy::ValueType::Float8;
        auto size = stride * count;
        command->Payload.resize(size);

        // position
        {
            auto src = positions->buffer.data();
            auto p = command->Payload.data();
            for (UINT i = 0; i < count; ++i,
                      src += positions->Stride(),
                      p += stride)
            {
                *(DirectX::XMFLOAT3 *)p = *(DirectX::XMFLOAT3 *)src;
            }
        }

        {
            auto normal = sceneMesh->GetVertices(hierarchy::Semantics::Normal);
            if (normal)
            {
                auto src = normal->buffer.data();
                auto p = command->Payload.data() + (4 * 3); // offset float3
                for (UINT i = 0; i < count; ++i,
                          src += normal->Stride(),
                          p += stride)
                {
                    *(DirectX::XMFLOAT3 *)p = *(DirectX::XMFLOAT3 *)src;
                }
            }
        }

        {
            auto uv = sceneMesh->GetVertices(hierarchy::Semantics::TexCoord);
            if (uv)
            {
                auto src = uv->buffer.data();
                auto p = command->Payload.data() + (4 * 6); // offset float3
                for (UINT i = 0; i < count; ++i,
                          src += uv->Stride(),
                          p += stride)
                {
                    *(DirectX::XMFLOAT2 *)p = *(DirectX::XMFLOAT2 *)src;
                }
            }
        }

        auto resource = ResourceItem::CreateDefault(device, size);
        command->UsePayload(resource, stride);
        uploader->EnqueueUpload(command);
        return resource;
    }

    throw;
    return {};
}

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh)
{
    auto found = m_meshMap.find(sceneMesh);
    if (found != m_meshMap.end())
    {
        return found->second;
    }

    auto gpuMesh = std::make_shared<Mesh>();

    // vertices
    {
        auto resource = CreateResourceItem(device, m_uploader, sceneMesh);
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
            m_uploader->EnqueueUpload(resource, indices->buffer.data(), (UINT)indices->buffer.size(), indices->Stride());
        }
    }

    m_meshMap.insert(std::make_pair(sceneMesh, gpuMesh));
    return gpuMesh;
}

std::shared_ptr<class Texture> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneImagePtr &image)
{
    auto found = m_textureMap.find(image);
    if (found != m_textureMap.end())
    {
        return found->second;
    }

    auto gpuTexture = std::make_shared<Texture>();

    {
        auto resource = ResourceItem::CreateDefaultImage(device, image->width, image->height);
        gpuTexture->ImageBuffer(resource);
        m_uploader->EnqueueUpload(resource, image->buffer.data(), (UINT)image->buffer.size(), image->width * 4);
    }

    m_textureMap.insert(std::make_pair(image, gpuTexture));
    return gpuTexture;
}

} // namespace d12u

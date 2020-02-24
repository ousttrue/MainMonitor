#include "SceneMapper.h"
#include "ResourceItem.h"
#include "Mesh.h"
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
    auto positions = sceneMesh->GetVertices(hierarchy::Semantics::PositionNormalTexCoord);
    if (positions)
    {
        auto resource = ResourceItem::CreateDefault(device, (UINT)positions->buffer.size());
        uploader->EnqueueUpload(resource, positions->buffer.data(), (UINT)positions->buffer.size(), positions->Stride());
        return resource;
    }

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
        auto resource = ResourceItem::CreateDefault(device, size);
        auto src = positions->buffer.data();
        auto p = command->Payload.data();
        for (UINT i = 0; i < count; ++i,
                  src += positions->Stride(),
                  p += stride)
        {
            *(DirectX::XMFLOAT3 *)p = *(DirectX::XMFLOAT3 *)src;
        }
        command->UsePayload(resource, stride);
        uploader->EnqueueUpload(command);
        return resource;
    }

    return {};
}

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh)
{
    auto found = m_modelMeshMap.find(sceneMesh);
    if (found != m_modelMeshMap.end())
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
        auto resource = ResourceItem::CreateDefault(device, (UINT)indices->buffer.size());
        gpuMesh->IndexBuffer(resource);
        m_uploader->EnqueueUpload(resource, indices->buffer.data(), (UINT)indices->buffer.size(), indices->Stride());
    }

    m_modelMeshMap.insert(std::make_pair(sceneMesh, gpuMesh));
    return gpuMesh;
}
} // namespace d12u

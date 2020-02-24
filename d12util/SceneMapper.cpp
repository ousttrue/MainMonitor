#include "SceneMapper.h"
#include "ResourceItem.h"
#include "Mesh.h"
#include "Uploader.h"

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

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh)
{
    auto found = m_modelMeshMap.find(sceneMesh);
    if (found != m_modelMeshMap.end())
    {
        return found->second;
    }

    auto gpuMesh = std::make_shared<Mesh>();

    auto positions = sceneMesh->GetVertices(hierarchy::Semantics::PositionNormalTexCoord);
    if (positions)
    {
        auto resource = ResourceItem::CreateDefault(device, (UINT)positions->buffer.size());
        gpuMesh->VertexBuffer(resource);
        m_uploader->EnqueueUpload(resource, positions->buffer.data(), (UINT)positions->buffer.size(), positions->Stride());
    }
    else
    {
        // convert
        auto a = 0;
    }

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

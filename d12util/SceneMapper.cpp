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

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::SceneMesh> &model)
{
    auto found = m_modelMeshMap.find(model);
    if (found != m_modelMeshMap.end())
    {
        return found->second;
    }

    auto mesh = std::make_shared<Mesh>();

    if (model->VerticesByteLength())
    {
        auto vertices = ResourceItem::CreateDefault(device, model->VerticesByteLength());
        mesh->VertexBuffer(vertices);
        m_uploader->EnqueueUpload(vertices, model->Vertices(), model->VerticesByteLength(), model->VertexStride());
    }

    if (model->IndicesByteLength())
    {
        auto indices = ResourceItem::CreateDefault(device, model->IndicesByteLength());
        mesh->IndexBuffer(indices);
        m_uploader->EnqueueUpload(indices, model->Indices(), model->IndicesByteLength(), model->IndexStride());
    }

    m_modelMeshMap.insert(std::make_pair(model, mesh));
    return mesh;
}
} // namespace d12u

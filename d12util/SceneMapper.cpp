#include "SceneMapper.h"
#include "ResourceItem.h"
#include "Mesh.h"
#include "Texture.h"
#include "Uploader.h"
#include "RootSignature.h"
#include "Shader.h"
#include <hierarchy.h>
#include <DirectXMath.h>
#include <plog/Log.h>

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

std::shared_ptr<Mesh> SceneMapper::GetOrCreate(const ComPtr<ID3D12Device> &device,
                                               const std::shared_ptr<hierarchy::SceneMesh> &sceneMesh,
                                               RootSignature *rootSignature)
{
    auto found = m_meshMap.find(sceneMesh);
    if (found != m_meshMap.end())
    {
        return found->second;
    }

    if (!rootSignature)
    {
        return nullptr;
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
        // auto resource = CreateResourceItem(device, m_uploader, sceneMesh, shader->inputLayout(), shader->inputLayoutCount());
        auto dstStride = 0;
        int inputLayoutCount;
        auto inputLayout = shader->inputLayout(&inputLayoutCount);
        for (int i = 0; i < inputLayoutCount; ++i)
        {
            dstStride += GetStride(inputLayout[i].Format);
        }

        // vertices
        auto vertices = sceneMesh->vertices;
        if (vertices->stride != dstStride)
        {
            LOGE << "buffer stride difference with shader stride";
            return nullptr;
        }

        std::shared_ptr<ResourceItem> resource;
        if (vertices->isDynamic)
        {
            resource = ResourceItem::CreateUpload(device, (UINT)vertices->buffer.size(), sceneMesh->name.c_str());
            // not enqueue
        }
        else if (sceneMesh->skin)
        {
            resource = ResourceItem::CreateUpload(device, (UINT)vertices->buffer.size(), sceneMesh->name.c_str());
            // not enqueue
        }
        else
        {
            resource = ResourceItem::CreateDefault(device, (UINT)vertices->buffer.size(), sceneMesh->name.c_str());
            m_uploader->EnqueueUpload(resource, vertices->buffer.data(), (UINT)vertices->buffer.size(), vertices->stride);
        }

        if (!resource)
        {
            // fail
            return nullptr;
        }
        gpuMesh->VertexBuffer(resource);
    }

    // indices
    auto indices = sceneMesh->indices;
    if (indices)
    {
        if (indices->isDynamic)
        {
            auto resource = ResourceItem::CreateUpload(device, (UINT)indices->buffer.size(), sceneMesh->name.c_str());
            gpuMesh->IndexBuffer(resource);
            // not enqueue
        }
        else
        {
            auto resource = ResourceItem::CreateDefault(device, (UINT)indices->buffer.size(), sceneMesh->name.c_str());
            gpuMesh->IndexBuffer(resource);
            m_uploader->EnqueueUpload(resource, indices->buffer.data(), (UINT)indices->buffer.size(), indices->stride);
        }
    }

    m_meshMap.insert(std::make_pair(sceneMesh, gpuMesh));
    return gpuMesh;
}

} // namespace d12u

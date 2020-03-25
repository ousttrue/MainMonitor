#include "Renderer.h"
#include "Gui/ImGuiDX12.h"
#include <d12util.h>

#include <DrawList.h>
#include <SceneView.h>

#include <plog/Log.h>
#include <imgui.h>

const UINT BACKBUFFER_COUNT = 2;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class Impl
{
    std::unique_ptr<d12u::SwapChain> m_swapchain;
    std::unique_ptr<d12u::RenderTargetChain> m_backbuffer;
    int m_width = 0;
    int m_height = 0;

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<d12u::CommandQueue> m_queue;
    std::unique_ptr<d12u::RootSignature> m_rootSignature;
    std::unique_ptr<d12u::CommandList> m_commandlist;
    std::unique_ptr<d12u::SceneMapper> m_sceneMapper;

    ImGuiDX12 m_imguiDX12;

    // scene
    std::unique_ptr<hierarchy::SceneLight> m_light;

public:
    Impl(int maxModelCount)
        : m_queue(new d12u::CommandQueue),
          m_swapchain(new d12u::SwapChain),
          m_backbuffer(new d12u::RenderTargetChain),
          m_commandlist(new d12u::CommandList),
          m_rootSignature(new d12u::RootSignature),
          m_sceneMapper(new d12u::SceneMapper),
          m_light(new hierarchy::SceneLight)
    {
    }

    void Initialize(HWND hwnd)
    {
        assert(!m_device);

        ComPtr<IDXGIFactory4> factory;
        d12u::ThrowIfFailed(CreateDXGIFactory2(d12u::GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter = d12u::GetHardwareAdapter(factory.Get());
        d12u::ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));

        m_queue->Initialize(m_device);
        m_swapchain->Initialize(factory, m_queue->Get(), hwnd, BACKBUFFER_COUNT);
        m_backbuffer->Initialize(m_swapchain->Get(), m_device, BACKBUFFER_COUNT);
        m_sceneMapper->Initialize(m_device);
        m_commandlist->InitializeDirect(m_device);
        m_rootSignature->Initialize(m_device);

        m_imguiDX12.Initialize(m_device.Get(), BACKBUFFER_COUNT);

        //
        // settings
        // https://blog.techlab-xe.net/dx12-debug-id3d12infoqueue/
        //
        ComPtr<ID3D12InfoQueue> infoQueue;
        m_device.As(&infoQueue);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO};
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue->PushStorageFilter(&filter);

        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }

    void BeginFrame(HWND hwnd, int width, int height)
    {
        UpdateBackbuffer(hwnd, width, height);
        m_sceneMapper->Update(m_device);
        m_rootSignature->Update(m_device);

        // new frame
        m_commandlist->Reset(nullptr);
    }

    void EndFrame()
    {
        auto commandList = m_commandlist->Get();
        auto frameIndex = m_swapchain->CurrentFrameIndex();

        // barrier
        m_backbuffer->Begin(frameIndex, commandList, nullptr);

        ImGui::Render();
        m_imguiDX12.RenderDrawData(commandList.Get(), ImGui::GetDrawData());

        m_backbuffer->End(frameIndex, commandList);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_swapchain->Present();
        m_queue->SyncFence(callbacks);
    }

    size_t ViewTextureID(const hierarchy::SceneViewPtr &sceneView)
    {
        // view texture for current frame
        auto viewRenderTarget = m_sceneMapper->GetOrCreate(sceneView);
        auto resource = viewRenderTarget->Resource(m_swapchain->CurrentFrameIndex());
        size_t texture = resource ? m_imguiDX12.GetOrCreateTexture(m_device.Get(), resource->renderTarget.Get()) : -1;
        return texture;
    }

    void View(const hierarchy::SceneViewPtr &sceneView)
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreate(sceneView);

        UpdateNodes(sceneView->Drawlist);

        UpdateView(viewRenderTarget, sceneView);

        DrawView(m_commandlist->Get(), m_swapchain->CurrentFrameIndex(), viewRenderTarget, sceneView);
    }

private:
    void UpdateBackbuffer(HWND hwnd, int width, int height)
    {
        if (m_width != width || m_height != height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_backbuffer->Release(); // require before resize
            m_swapchain->Resize(m_queue->Get(),
                                hwnd, BACKBUFFER_COUNT, width, height);
            m_backbuffer->Initialize(m_swapchain->Get(), m_device, BACKBUFFER_COUNT);

            m_width = width;
            m_height = height;
        }
    }

    void UpdateNodes(const hierarchy::DrawList &drawlist)
    {
        // nodes
        for (auto &drawNode : drawlist.Nodes)
        {
            m_rootSignature->GetDrawConstantsBuffer(drawNode.NodeID)->b1World = drawNode.WorldMatrix;
        }

        // skins
        for (auto &drawMesh : drawlist.Meshes)
        {
            auto mesh = drawMesh.Mesh;
            auto drawable = m_sceneMapper->GetOrCreate(m_device, drawMesh.Mesh, nullptr);
            if (drawable)
            {
                auto skin = mesh->skin;
                if (skin)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(
                        skin->cpuSkiningBuffer.data(), (uint32_t)skin->cpuSkiningBuffer.size(), mesh->vertices->stride);
                }
                if (drawMesh.Vertices.Ptr)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(drawMesh.Vertices.Ptr, drawMesh.Vertices.Bytes, drawMesh.Vertices.Stride);
                }
                if (drawMesh.Indices.Ptr)
                {
                    drawable->IndexBuffer()->MapCopyUnmap(drawMesh.Indices.Ptr, drawMesh.Indices.Bytes, drawMesh.Indices.Stride);
                }
            }
        }

        m_rootSignature->UploadDrawConstantsBuffer();
    }

    void UpdateView(const std::shared_ptr<d12u::RenderTargetChain> &viewRenderTarget,
                    const hierarchy::SceneViewPtr &sceneView)
    {
        {
            auto buffer = m_rootSignature->GetViewConstantsBuffer(0);
            buffer->b0Projection = falg::size_cast<DirectX::XMFLOAT4X4>(sceneView->Projection);
            buffer->b0View = falg::size_cast<DirectX::XMFLOAT4X4>(sceneView->View);
            buffer->b0LightDir = m_light->LightDirection;
            buffer->b0LightColor = m_light->LightColor;
            buffer->b0CameraPosition = falg::size_cast<DirectX::XMFLOAT3>(sceneView->CameraPosition);
            buffer->b0ScreenSizeFovY = {(float)sceneView->Width, (float)sceneView->Height, sceneView->CameraFovYRadians};
            m_rootSignature->UploadViewConstantsBuffer();
        }

        if (viewRenderTarget->Resize(sceneView->Width, sceneView->Height))
        {
            // clear all
            for (UINT i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                auto resource = viewRenderTarget->Resource(i);
                if (resource)
                {
                    m_imguiDX12.Remove(resource->renderTarget.Get());
                }
            }
            viewRenderTarget->Initialize(sceneView->Width, sceneView->Height, m_device, BACKBUFFER_COUNT);
        }
    }

    void DrawView(const ComPtr<ID3D12GraphicsCommandList> &commandList, int frameIndex,
                  const std::shared_ptr<d12u::RenderTargetChain> &viewRenderTarget,
                  const hierarchy::SceneViewPtr &sceneView)
    {
        // clear
        if (viewRenderTarget->Resource(frameIndex))
        {
            viewRenderTarget->Begin(frameIndex, commandList, sceneView->ClearColor.data());

            // global settings
            m_rootSignature->Begin(commandList);

            for (auto &drawMesh : sceneView->Drawlist.Meshes)
            {
                m_rootSignature->SetDrawDescriptorTable(commandList, drawMesh.NodeID);
                DrawMesh(commandList, drawMesh);
            }

            viewRenderTarget->End(frameIndex, commandList);
        }
    }

    void DrawMesh(const ComPtr<ID3D12GraphicsCommandList> &commandList, const hierarchy::DrawList::MeshInfo &info)
    {
        auto &mesh = info.Mesh;
        if (!mesh)
        {
            return;
        }

        auto drawable = m_sceneMapper->GetOrCreate(m_device, mesh, m_rootSignature.get());
        if (!drawable)
        {
            return;
        }
        if (!drawable->IsDrawable(m_commandlist.get()))
        {
            return;
        }

        // for (auto &submesh : mesh->submeshes)
        {
            auto &submesh = mesh->submeshes[info.SubmeshIndex];
            auto material = m_rootSignature->GetOrCreate(m_device, submesh.material);

            // texture setup
            if (submesh.material->colorImage)
            {
                auto [texture, textureSlot] = m_rootSignature->GetOrCreate(m_device, submesh.material->colorImage,
                                                                           m_sceneMapper->GetUploader());
                if (texture)
                {
                    if (texture->IsDrawable(m_commandlist.get(), 0))
                    {
                        m_rootSignature->SetTextureDescriptorTable(commandList, textureSlot);
                    }
                }
            }

            if (material->Set(commandList))
            {
                m_commandlist->Get()->DrawIndexedInstanced(submesh.drawCount, 1, submesh.drawOffset, 0, 0);
            }
        }
    }
};

Renderer::Renderer(int maxModelCount)
    : m_impl(new Impl(maxModelCount))
{
}

Renderer::~Renderer()
{
    delete m_impl;
}

void Renderer::Initialize(void *hwnd)
{
    m_impl->Initialize((HWND)hwnd);
}

void Renderer::BeginFrame(void *hwnd, int width, int height)
{
    m_impl->BeginFrame((HWND)hwnd, width, height);
}

void Renderer::EndFrame()
{
    m_impl->EndFrame();
}

size_t Renderer::ViewTextureID(const std::shared_ptr<hierarchy::SceneView> &view)
{
    return m_impl->ViewTextureID(view);
}

void Renderer::View(const hierarchy::SceneViewPtr &view)
{
    m_impl->View(view);
}

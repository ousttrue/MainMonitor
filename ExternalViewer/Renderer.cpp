#include "Renderer.h"
#include "ImGuiDX12.h"
#include <d12util.h>
#include <OrbitCamera.h>
#include <DrawList.h>

#include <plog/Log.h>
#include <imgui.h>

#define YAP_ENABLE
#include "YAP.h"

using namespace d12u;

const UINT BACKBUFFER_COUNT = 2;

class Impl
{
    std::unique_ptr<SwapChain> m_swapchain;
    std::unique_ptr<RenderTargetChain> m_backbuffer;
    int m_width = 0;
    int m_height = 0;

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<CommandList> m_commandlist;
    std::unique_ptr<SceneMapper> m_sceneMapper;

    ImGuiDX12 m_imguiDX12;

    hierarchy::SceneViewPtr m_sceneView;

    // scene
    std::unique_ptr<hierarchy::SceneLight> m_light;

    float m_clearColor[4] = {
        0.2f,
        0.2f,
        0.3f,
        1.0f};

public:
    Impl(int maxModelCount)
        : m_queue(new CommandQueue),
          m_swapchain(new SwapChain),
          m_backbuffer(new RenderTargetChain),
          m_commandlist(new CommandList),
          m_rootSignature(new RootSignature),
          m_sceneMapper(new SceneMapper),
          m_light(new hierarchy::SceneLight),
          m_sceneView(new hierarchy::SceneView)
    {
    }

    void Initialize(HWND hwnd)
    {
        assert(!m_device);

        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter = GetHardwareAdapter(factory.Get());
        ThrowIfFailed(D3D12CreateDevice(
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

    void OnFrame(HWND hwnd, int width, int height,
                 hierarchy::DrawList *drawlist)
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreate(m_sceneView);

        {
            YAP::ScopedSection(Update);

            // d3d
            UpdateBackbuffer(hwnd, width, height);
            UpdateNodes(drawlist);
            m_sceneMapper->Update(m_device);
            m_rootSignature->Update(m_device);
        }
        {
            YAP::ScopedSection(Draw);
            Draw(viewRenderTarget, drawlist);
        }
    }

    size_t ViewTextureID()
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreate(m_sceneView);
        auto resource = viewRenderTarget->Resource(m_swapchain->CurrentFrameIndex());
        auto texture = resource ? m_imguiDX12.GetOrCreateTexture(m_device.Get(), resource->renderTarget.Get()) : 0;
        return texture;
    }

    void UpdateViewResource(const screenstate::ScreenState &viewState, const OrbitCamera *camera)
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreate(m_sceneView);
        Update3DViewResource(viewRenderTarget, viewState, camera);
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

    void Update3DViewResource(const std::shared_ptr<RenderTargetChain> &viewRenderTarget,
                              const screenstate::ScreenState &viewState,
                              const OrbitCamera *camera)
    {
        // resource
        {
            auto buffer = m_rootSignature->GetSceneConstantsBuffer(0);
            buffer->b0Projection = falg::size_cast<DirectX::XMFLOAT4X4>(camera->state.projection);
            buffer->b0View = falg::size_cast<DirectX::XMFLOAT4X4>(camera->state.view);
            buffer->b0LightDir = m_light->LightDirection;
            buffer->b0LightColor = m_light->LightColor;
            buffer->b0CameraPosition = falg::size_cast<DirectX::XMFLOAT3>(camera->state.position);
            buffer->b0ScreenSizeFovY = {(float)viewState.Width, (float)viewState.Height, camera->state.fovYRadians};
            m_rootSignature->UploadSceneConstantsBuffer();
        }

        if (viewRenderTarget->Resize(viewState.Width, viewState.Height))
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
            viewRenderTarget->Initialize(viewState.Width, viewState.Height, m_device, BACKBUFFER_COUNT);
        }
    }

    void UpdateNodes(hierarchy::DrawList *drawlist)
    {
        // nodes
        for (auto &drawNode : drawlist->Nodes)
        {
            m_rootSignature->GetNodeConstantsBuffer(drawNode.NodeID)->b1World = drawNode.WorldMatrix;
        }
        m_rootSignature->UploadNodeConstantsBuffer();

        // skins
        for (auto &drawMesh : drawlist->Meshes)
        {
            auto mesh = drawMesh.Mesh;
            auto skin = mesh->skin;
            auto drawable = m_sceneMapper->GetOrCreate(m_device, drawMesh.Mesh, nullptr);
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

    //
    // command
    //
    void Draw(const std::shared_ptr<RenderTargetChain> &viewRenderTarget,
              const hierarchy::DrawList *drawlist)
    {
        // new frame
        m_commandlist->Reset(nullptr);
        auto commandList = m_commandlist->Get();

        auto frameIndex = m_swapchain->CurrentFrameIndex();

        // clear
        if (viewRenderTarget->Resource(frameIndex))
        {
            viewRenderTarget->Begin(frameIndex, commandList, m_clearColor);

            // global settings
            m_rootSignature->Begin(commandList);

            for (auto &drawMesh : drawlist->Meshes)
            {
                m_rootSignature->SetNodeDescriptorTable(commandList, drawMesh.NodeID);
                DrawMesh(commandList, drawMesh.Mesh);
            }

            viewRenderTarget->End(frameIndex, commandList);
        }

        // barrier
        m_backbuffer->Begin(frameIndex, commandList, m_clearColor);

        ImGui::Render();
        m_imguiDX12.RenderDrawData(commandList.Get(), ImGui::GetDrawData());

        m_backbuffer->End(frameIndex, commandList);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_swapchain->Present();
        m_queue->SyncFence(callbacks);
    }

    void DrawMesh(const ComPtr<ID3D12GraphicsCommandList> &commandList, const hierarchy::SceneMeshPtr &mesh)
    {
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

        int offset = 0;
        for (auto &submesh : mesh->submeshes)
        {
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

            if (material->m_shader->Set(commandList))
            {
                m_commandlist->Get()->DrawIndexedInstanced(submesh.draw_count, 1, offset, 0, 0);
            }

            offset += submesh.draw_count;
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

void Renderer::OnFrame(void *hwnd, int width, int height, hierarchy::DrawList *drawlist)
{
    m_impl->OnFrame((HWND)hwnd, width, height, drawlist);
}

size_t Renderer::ViewTextureID()
{
    return m_impl->ViewTextureID();
}

void Renderer::UpdateViewResource(const screenstate::ScreenState &viewState, const OrbitCamera *camera)
{
    m_impl->UpdateViewResource(viewState, camera);
}

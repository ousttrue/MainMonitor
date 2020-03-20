#include "Renderer.h"
#include "Gizmo.h"
#include "Gui.h"
#include <d12util.h>
#include <unordered_map>
#include <OrbitCamera.h>
#include <hierarchy.h>

#include <plog/Log.h>
#include <imgui.h>
#include <dxgi.h>
#include <memory>

using namespace d12u;

const UINT BACKBUFFER_COUNT = 2;

class Impl
{
    screenstate::ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_swapchain;
    std::unique_ptr<RenderTargetChain> m_backbuffer;
    std::unique_ptr<RenderTargetChain> m_view;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<CommandList> m_commandlist;
    std::unique_ptr<SceneMapper> m_sceneMapper;

    std::unique_ptr<Gui> m_imgui;

    std::unique_ptr<hierarchy::Scene> m_scene;

    // scene
    std::unique_ptr<OrbitCamera> m_camera;
    std::unique_ptr<hierarchy::SceneLight> m_light;

    // gizmo
    Gizmo m_gizmo;

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
          m_view(new RenderTargetChain),
          m_commandlist(new CommandList),
          m_rootSignature(new RootSignature),
          m_sceneMapper(new SceneMapper),
          m_camera(new OrbitCamera),
          m_light(new hierarchy::SceneLight),
          m_scene(new hierarchy::Scene)
    {
        m_camera->zNear = 0.01f;
    }

    void log(const char *msg)
    {
        if (m_imgui)
        {
            m_imgui->Log(msg);
        }
    }

    const std::unique_ptr<hierarchy::Scene> &Scene() const { return m_scene; }

    void Initialize(HWND hwnd)
    {
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
        // m_view->InitializeIfSizeChanged(640, 480, m_device, BACKBUFFER_COUNT);
        m_sceneMapper->Initialize(m_device);
        m_commandlist->InitializeDirect(m_device);
        m_rootSignature->Initialize(m_device);

        m_imgui.reset(new Gui(m_device, BACKBUFFER_COUNT, hwnd));
    }

    void OnFrame(HWND hwnd, const screenstate::ScreenState &state)
    {
        if (!m_device)
        {
            // first time
            Initialize(hwnd);

            m_rootSignature->GetNodeConstantsBuffer(m_gizmo.GetNodeID())->b1World = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1};
        }

        Update(hwnd, state);
        Draw(state);
    }

private:
    void Update(HWND hwnd, const screenstate::ScreenState &state)
    {
        m_sceneMapper->Update(m_device);

        // update
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_backbuffer->Release(); // require before resize
            m_swapchain->Resize(m_queue->Get(),
                                hwnd, BACKBUFFER_COUNT, state.Width, state.Height);
            m_backbuffer->Initialize(m_swapchain->Get(), m_device, BACKBUFFER_COUNT);
        }

        m_imgui->BeginFrame(state);

        // 3d view
        auto resource = m_view->Resource(m_swapchain->CurrentFrameIndex());
        // if (resource)
        {
            auto texture = resource ? m_imgui->GetOrCreateTexture(m_device.Get(), resource->renderTarget.Get()) : 0;
            screenstate::ScreenState viewState;
            if (m_imgui->View(state, texture, &viewState))
            {
                m_camera->Update(viewState);
                {
                    auto buffer = m_rootSignature->GetSceneConstantsBuffer(0);
                    buffer->b0Projection = falg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.projection);
                    buffer->b0View = falg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.view);
                    buffer->b0LightDir = m_light->LightDirection;
                    buffer->b0LightColor = m_light->LightColor;
                    buffer->b0CameraPosition = falg::size_cast<DirectX::XMFLOAT3>(m_camera->state.position);
                    buffer->b0ScreenSizeFovY = {(float)state.Width, (float)state.Height, m_camera->state.fovYRadians};
                    m_rootSignature->UploadSceneConstantsBuffer();
                }
            }
            if (m_view->Resize(viewState.Width, viewState.Height))
            {
                // clear all
                for (UINT i = 0; i < BACKBUFFER_COUNT; ++i)
                {
                    auto resource = m_view->Resource(i);
                    if (resource)
                    {
                        m_imgui->Remove(resource->renderTarget.Get());
                    }
                }
                m_view->Initialize(viewState.Width, viewState.Height, m_device, BACKBUFFER_COUNT);
            }
        }

        m_imgui->Update(m_scene.get(), m_clearColor);

        int nodeCount;
        auto nodes = m_scene->GetRootNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            auto root = nodes[i];
            root->UpdateWorld();
            UpdateNode(root);
        }

        m_rootSignature->UploadNodeConstantsBuffer();
        m_lastState = state;

        // gizmo: upload
        {
            m_gizmo.Begin(state, m_camera->state);
            auto selected = m_imgui->Selected();
            if (selected)
            {
                // if (selected->EnableGizmo())
                {
                    auto parent = selected->Parent();
                    m_gizmo.Transform(selected->ID(),
                                      selected->Local(),
                                      parent ? parent->World() : falg::Transform{});
                }
            }
            auto buffer = m_gizmo.End();

            auto drawable = m_sceneMapper->GetOrCreate(m_device, m_gizmo.GetMesh(), m_rootSignature.get());
            drawable->VertexBuffer()->MapCopyUnmap(buffer.pVertices, buffer.verticesBytes, buffer.vertexStride);
            drawable->IndexBuffer()->MapCopyUnmap(buffer.pIndices, buffer.indicesBytes, buffer.indexStride);
        }

        // shader udpate
        m_rootSignature->Update(m_device);
    }

    void
    UpdateNode(const hierarchy::SceneNodePtr &node)
    {
        // auto current = node->Local() * parent;
        m_rootSignature->GetNodeConstantsBuffer(node->ID())->b1World = node->World().RowMatrix();

        auto mesh = node->Mesh();
        if (mesh)
        {
            auto skin = mesh->skin;
            if (skin)
            {
                // skining matrix
                auto &vertices = mesh->vertices;
                skin->Update(vertices->buffer.data(), vertices->stride, vertices->Count());
                auto drawable = m_sceneMapper->GetOrCreate(m_device, mesh, nullptr);
                if (drawable)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(skin->cpuSkiningBuffer.data(), (uint32_t)skin->cpuSkiningBuffer.size(), mesh->vertices->stride);
                }
            }
        }

        int childCount;
        auto children = node->GetChildren(&childCount);
        for (int i = 0; i < childCount; ++i)
        {
            UpdateNode(children[i]);
        }
    }

    //
    // command
    //
    void Draw(const screenstate::ScreenState &state)
    {
        // new frame
        m_commandlist->Reset(nullptr);
        auto commandList = m_commandlist->Get();

        // clear
        auto frameIndex = m_swapchain->CurrentFrameIndex();
        m_view->Begin(frameIndex, commandList, m_clearColor);
        {
            // global settings
            m_rootSignature->Begin(commandList);

            // model
            int nodeCount;
            auto nodes = m_scene->GetRootNodes(&nodeCount);
            for (int i = 0; i < nodeCount; ++i)
            {
                DrawNode(commandList, nodes[i]);
            }

            // gizmo: draw
            {
                m_rootSignature->SetNodeDescriptorTable(commandList, m_gizmo.GetNodeID());
                auto drawable = m_sceneMapper->GetOrCreate(m_device, m_gizmo.GetMesh(), m_rootSignature.get());
                if (drawable->IsDrawable(m_commandlist.get()))
                {
                    DrawMesh(commandList, m_gizmo.GetMesh());
                }
            }

            m_view->End(frameIndex, commandList);
        }

        // barrier
        m_backbuffer->Begin(frameIndex, commandList, m_clearColor);
        m_imgui->EndFrame(commandList);
        m_backbuffer->End(frameIndex, commandList);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_swapchain->Present();
        m_queue->SyncFence(callbacks);
    }

    void DrawNode(const ComPtr<ID3D12GraphicsCommandList> &commandList, const hierarchy::SceneNodePtr &node)
    {
        m_rootSignature->SetNodeDescriptorTable(commandList, node->ID());

        auto mesh = node->Mesh();
        if (mesh)
        {
            DrawMesh(commandList, mesh);
        }

        int childCount;
        auto children = node->GetChildren(&childCount);
        for (int i = 0; i < childCount; ++i)
        {
            DrawNode(commandList, children[i]);
        }
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

void Renderer::OnFrame(void *hwnd, const screenstate::ScreenState &state)
{
    m_impl->OnFrame((HWND)hwnd, state);
}

hierarchy::Scene *Renderer::GetScene()
{
    return m_impl->Scene().get();
}

void Renderer::log(const char *msg)
{
    m_impl->log(msg);
}

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

namespace plog
{

template <class Formatter>
class MyAppender : public IAppender
{
public:
    void write(const Record &record) override
    {
    }
};

} // namespace plog

class Impl
{
    screenstate::ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
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
          m_rt(new SwapChain(2)),
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
        m_rt->Initialize(factory, m_queue->Get(), hwnd);
        m_sceneMapper->Initialize(m_device);
        m_commandlist->InitializeDirect(m_device);
        m_rootSignature->Initialize(m_device);

        m_imgui.reset(new Gui(m_device, m_rt->BufferCount(), hwnd));
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
            m_rt->Resize(m_queue->Get(),
                         hwnd, state.Width, state.Height);
        }

        m_imgui->BeginFrame(state);
        if (m_imgui->Update(m_scene.get(), m_clearColor))
        {
            // consume input event by imgui
        }
        else
        {
            m_camera->Update(state);
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

        int nodeCount;
        auto nodes = m_scene->GetRootNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            UpdateNode(nodes[i], falg::Transform());
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
                                      selected->Local,
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

    void UpdateNode(const hierarchy::SceneNodePtr &node, const falg::Transform &parent)
    {
        auto current = node->Local * parent;
        m_rootSignature->GetNodeConstantsBuffer(node->ID())->b1World = current.Matrix();

        int childCount;
        auto children = node->GetChildren(&childCount);
        for (int i = 0; i < childCount; ++i)
        {
            UpdateNode(children[i], current);
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
        auto &rt = m_rt->Begin(commandList, m_clearColor);

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
                int offset = 0;
                for (auto &submesh : m_gizmo.GetMesh()->submeshes)
                {
                    auto material = m_rootSignature->GetOrCreate(m_device, submesh.material);
                    if (material->m_shader->Set(commandList))
                    {
                        m_commandlist->Get()->DrawIndexedInstanced(submesh.draw_count, 1, offset, 0, 0);
                    }

                    offset += submesh.draw_count;
                }
            }
        }

        m_imgui->EndFrame(commandList);

        // barrier
        m_rt->End(commandList, rt);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }

    void DrawNode(const ComPtr<ID3D12GraphicsCommandList> &commandList, const hierarchy::SceneNodePtr &node)
    {
        m_rootSignature->SetNodeDescriptorTable(commandList, node->ID());

        int meshCount;
        auto meshes = node->GetMeshes(&meshCount);
        for (int i = 0; i < meshCount; ++i)
        {
            DrawMesh(commandList, meshes[i]);
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
                    else
                    {
                        // wait upload
                        continue;
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

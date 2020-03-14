#include "Renderer.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>
#include <dxgi.h>
#include <plog/Log.h>
#include <imgui.h>

#include <OrbitCamera.h>
#include "Gizmo.h"

#include "SceneLight.h"
#include "Scene.h"
#include "Gui.h"

using namespace d12u;

#include <shobjidl.h>
std::wstring OpenFileDialog(const std::wstring &folder)
{
    ComPtr<IFileOpenDialog> pFileOpen;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                IID_PPV_ARGS(&pFileOpen))))
    {
        return L"";
    }

    COMDLG_FILTERSPEC fileTypes[] = {
        {L"gltf binary format", L"*.glb"},
        {L"all", L"*.*"},
    };
    if (FAILED(pFileOpen->SetFileTypes(_countof(fileTypes), fileTypes)))
    {
        return L"";
    }
    if (FAILED(pFileOpen->SetDefaultExtension(L".glb")))
    {
        return L"";
    }
    if (FAILED(pFileOpen->Show(NULL)))
    {
        return L"";
    }

    ComPtr<IShellItem> pItem;
    if (FAILED(pFileOpen->GetResult(&pItem)))
    {
        return L"";
    }

    PWSTR pszFilePath;
    if (FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
    {
        return L"";
    }
    std::wstring result(pszFilePath);
    CoTaskMemFree(pszFilePath);

    // DWORD len = GetCurrentDirectoryW(0, NULL);
    // std::vector<wchar_t> dir(len);
    // GetCurrentDirectoryW((DWORD)dir.size(), dir.data());
    // if(dir.back()==0)
    // {
    //     dir.pop_back();
    // }
    // std::wcout << std::wstring(dir.begin(), dir.end()) << std::endl;

    return result;
}

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

        m_camera->Update(state);
        {
            auto buffer = m_rootSignature->GetSceneConstantsBuffer(0);
            buffer->b0Projection = fpalg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.projection);
            buffer->b0View = fpalg::size_cast<DirectX::XMFLOAT4X4>(m_camera->state.view);
            buffer->b0LightDir = m_light->LightDirection;
            buffer->b0LightColor = m_light->LightColor;
            buffer->b0CameraPosition = fpalg::size_cast<DirectX::XMFLOAT3>(m_camera->state.position);
            buffer->b0ScreenSizeFovY = {(float)state.Width, (float)state.Height, m_camera->state.fovYRadians};
            m_rootSignature->UploadSceneConstantsBuffer();
        }

        m_gizmo.Begin(state, m_camera->state);

        int nodeCount;
        auto nodes = m_scene->GetNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            auto node = nodes[i];
            if (node)
            {
                m_rootSignature->GetNodeConstantsBuffer(node->ID())->b1World = node->TRS.Matrix();

                if (node->EnableGizmo())
                {
                    m_gizmo.Transform(i, node->TRS);
                }
            }
        }

        m_rootSignature->UploadNodeConstantsBuffer();
        m_lastState = state;

        // gizmo: upload
        {
            auto buffer = m_gizmo.End();

            auto drawable = m_sceneMapper->GetOrCreate(m_device, m_gizmo.GetMesh(), m_rootSignature.get());
            drawable->VertexBuffer()->MapCopyUnmap(buffer.pVertices, buffer.verticesBytes, buffer.vertexStride);
            drawable->IndexBuffer()->MapCopyUnmap(buffer.pIndices, buffer.indicesBytes, buffer.indexStride);
        }

        // shader udpate
        m_rootSignature->Update(m_device);
    }

    void DrawImGui()
    {
        //
        // imgui
        //
        {
            ImGui::Begin("main");
            if (ImGui::Button("open"))
            {
                auto path = OpenFileDialog(L"");
                m_scene->LoadFromPath(path);
                LOGI << "load: " << path;
            }
            ImGui::End();
        }
        {
            m_imgui->ShowLogger();
        }
        {
            ImGui::ShowDemoWindow();
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        static bool show_demo_window = true;
        static bool show_another_window = true;

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            // ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            // ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);    // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", m_clearColor); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
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
        auto nodes = m_scene->GetNodes(&nodeCount);
        for (int i = 0; i < nodeCount; ++i)
        {
            auto node = nodes[i];
            if (node)
            {
                m_rootSignature->SetNodeDescriptorTable(commandList, node->ID());

                int meshCount;
                auto meshes = node->GetMeshes(&meshCount);
                for (int j = 0; j < meshCount; ++j)
                {
                    auto mesh = meshes[j];
                    if (mesh)
                    {
                        auto drawable = m_sceneMapper->GetOrCreate(m_device, mesh, m_rootSignature.get());
                        if (drawable && drawable->IsDrawable(m_commandlist.get()))
                        {
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
                    }
                }
            }
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

        m_imgui->BeginFrame(state);
        DrawImGui();
        m_imgui->EndFrame(commandList);

        // barrier
        m_rt->End(commandList, rt);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_rt->Present();
        m_queue->SyncFence(callbacks);
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

#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>
#include <dxgi.h>
#include <imgui.h>
#include <examples/imgui_impl_win32.h>
#include <examples/imgui_impl_dx12.h>

std::string g_shaderSource =
#include "OpenVRRenderModel.hlsl"
    ;

using namespace d12u;

class DX12ImGui
{
    ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap;

public:
    DX12ImGui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd)
    {
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            {
                throw;
            }
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(device.Get(), bufferCount,
                            DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap.Get(),
                            g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                            g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);
    }

    ~DX12ImGui()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    bool show_demo_window = true;
    void OnFrame(ID3D12GraphicsCommandList *commandList)
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Render();

        commandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }
};

class Impl
{
    ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
    std::unique_ptr<Uploader> m_uploader;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<Camera> m_camera;

    std::unique_ptr<Heap> m_heap;
    d12u::ConstantBuffer<Camera::SceneConstantBuffer, 1> m_sceneConstant;
    d12u::ConstantBuffer<Model::ModelConstantBuffer, 64> m_modelConstant;

    std::unordered_map<std::shared_ptr<Model>, std::shared_ptr<Mesh>> m_modelMeshMap;

    std::unique_ptr<DX12ImGui> m_imgui;

    int m_maxModelCount = -1;

public:
    Impl(int maxModelCount)
        : m_queue(new CommandQueue),
          m_rt(new SwapChain(2)),
          m_uploader(new Uploader),
          m_pipeline(new Pipeline),
          m_camera(new Camera),
          m_heap(new Heap),
          m_maxModelCount(maxModelCount)
    {
    }

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
        m_uploader->Initialize(m_device);
        m_pipeline->Initialize(m_device, g_shaderSource, 2);
        m_sceneConstant.Initialize(m_device);
        m_modelConstant.Initialize(m_device);

        {
            HeapItem items[] = {
                {
                    .ConstantBuffer = &m_sceneConstant,
                    .Count = 1,
                },
                {
                    .ConstantBuffer = &m_modelConstant,
                    .Count = 64,
                },
            };
            m_heap->Initialize(m_device, _countof(items), items);
        }

        m_imgui.reset(new DX12ImGui(m_device, m_rt->BufferCount(), hwnd));
    }

    void OnFrame(HWND hwnd, const ScreenState &state,
                 const std::shared_ptr<Model> *models, int count)
    {
        if (!m_device)
        {
            // first time
            Initialize(hwnd);
        }

        m_uploader->Update(m_device);

        // update
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_rt->Resize(m_queue->Get(),
                         hwnd, state.Width, state.Height);
        }
        if (m_camera->OnFrame(state, m_lastState))
        {
            *m_sceneConstant.Get(0) = m_camera->Data;
            m_sceneConstant.CopyToGpu();
        }
        for (int i = 0; i < count; ++i)
        {
            auto model = models[i];
            if (model)
            {
                m_modelConstant.Get(model->ID())->world = model->Data.world;
            }
        }
        m_modelConstant.CopyToGpu();
        m_lastState = state;

        // command
        auto commandList = m_pipeline->Reset();
        float color[] = {
            0,
            0,
            0,
            1.0f,
        };
        auto &rt = m_rt->Begin(commandList->Get(), color);
        ID3D12DescriptorHeap *ppHeaps[] = {m_heap->Get()};
        commandList->Get()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // scene constant
        commandList->Get()->SetGraphicsRootDescriptorTable(0, m_heap->GpuHandle(0));

        // model
        for (int i = 0; i < count; ++i)
        {
            auto model = models[i];
            if (model)
            {
                auto mesh = GetOrCreate(model);
                // model constant
                commandList->Get()->SetGraphicsRootDescriptorTable(1, m_heap->GpuHandle(model->ID() + 1));
                // draw or barrier
                mesh->Command(commandList);
            }
        }

        // barrier
        m_rt->End(commandList->Get(), rt);

        // execute
        auto callbacks = commandList->Close();
        m_queue->Execute(commandList->Get());
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }

    std::shared_ptr<Mesh> GetOrCreate(const std::shared_ptr<Model> &model)
    {
        auto found = m_modelMeshMap.find(model);
        if (found != m_modelMeshMap.end())
        {
            return found->second;
        }

        auto mesh = std::make_shared<Mesh>();

        if (model->VerticesByteLength())
        {
            auto vertices = ResourceItem::CreateDefault(m_device, model->VerticesByteLength());
            mesh->VertexBuffer(vertices);
            m_uploader->EnqueueUpload(vertices, model->Vertices(), model->VerticesByteLength(), model->VertexStride());
        }

        if (model->IndicesByteLength())
        {
            auto indices = ResourceItem::CreateDefault(m_device, model->IndicesByteLength());
            mesh->IndexBuffer(indices);
            m_uploader->EnqueueUpload(indices, model->Indices(), model->IndicesByteLength(), model->IndexStride());
        }

        m_modelMeshMap.insert(std::make_pair(model, mesh));
        return mesh;
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

void Renderer::OnFrame(void *hwnd, const ScreenState &state,
                       const std::shared_ptr<class Model> *models, int count)
{
    m_impl->OnFrame((HWND)hwnd, state, models, count);
}

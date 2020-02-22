#include "Renderer.h"
#include <d12util.h>
#include <memory>
#include <unordered_map>
#include <dxgi.h>
#include <imgui.h>
#include "ImGuiWin32.h"
#include "ImGuiDX12.h"
#include "Camera.h"

std::string g_shaderSource =
#include "OpenVRRenderModel.hlsl"
    ;

using namespace d12u;

class Gui
{
    ImGuiWin32 m_win32;
    ImGuiDX12 m_dx12;

public:
    Gui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd)
    {
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
        m_win32.Init(hwnd);
        m_dx12.Initialize(device.Get(), bufferCount);

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

    ~Gui()
    {
        ImGui::DestroyContext();
    }

    bool show_demo_window = true;
    void OnFrame(const ComPtr<ID3D12GraphicsCommandList> &commandList,
                 HWND hwnd, const screenstate::ScreenState &state)
    {
        // Start the Dear ImGui frame
        m_win32.NewFrame(hwnd, state);
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Render();
        m_dx12.RenderDrawData(commandList.Get(), ImGui::GetDrawData());
    }
};

class Impl

{
    screenstate::ScreenState m_lastState = {};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<CommandQueue> m_queue;
    std::unique_ptr<SwapChain> m_rt;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<SceneMapper> m_sceneMapper;

    // scene
    std::unique_ptr<scngrph::Camera> m_camera;

    std::unique_ptr<Heap> m_heap;
    d12u::ConstantBuffer<scngrph::Camera::SceneConstantBuffer, 1> m_sceneConstant;
    d12u::ConstantBuffer<scngrph::Model::ModelConstantBuffer, 64> m_modelConstant;

    std::unique_ptr<Gui> m_imgui;

    int m_maxModelCount = -1;

public:
    Impl(int maxModelCount)
        : m_queue(new CommandQueue),
          m_rt(new SwapChain(2)),
          m_pipeline(new Pipeline),
          m_camera(new scngrph::Camera),
          m_heap(new Heap),
          m_sceneMapper(new SceneMapper),
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
        m_sceneMapper->Initialize(m_device);
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

        m_imgui.reset(new Gui(m_device, m_rt->BufferCount(), hwnd));
    }

    void OnFrame(HWND hwnd, const screenstate::ScreenState &state,
                 const std::shared_ptr<scngrph::Model> *models, int count)
    {
        if (!m_device)
        {
            // first time
            Initialize(hwnd);
        }

        m_sceneMapper->Update(m_device);

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
                auto mesh = m_sceneMapper->GetOrCreate(m_device, model);
                // model constant
                commandList->Get()->SetGraphicsRootDescriptorTable(1, m_heap->GpuHandle(model->ID() + 1));
                // draw or barrier
                mesh->Command(commandList);
            }
        }

        m_imgui->OnFrame(commandList->Get(), hwnd, state);

        // barrier
        m_rt->End(commandList->Get(), rt);

        // execute
        auto callbacks = commandList->Close();
        m_queue->Execute(commandList->Get());
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

void Renderer::OnFrame(void *hwnd, const screenstate::ScreenState &state,
                       const std::shared_ptr<scngrph::Model> *models, int count)
{
    m_impl->OnFrame((HWND)hwnd, state, models, count);
}

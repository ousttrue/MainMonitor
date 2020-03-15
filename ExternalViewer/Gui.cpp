#include "Gui.h"
#include <imgui.h>
#include "ImGuiImplScreenState.h"
#include "ImGuiDX12.h"
#include <plog/Log.h>

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

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

struct ExampleAppLog
{
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool AutoScroll;           // Keep scrolling if already at the bottom
    std::mutex m_mutex;

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    // may from another thread
    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        std::lock_guard<std::mutex> scoped(m_mutex);

        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        {
            std::lock_guard<std::mutex> scoped(m_mutex);
            const char *buf = Buf.begin();
            const char *buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have a random access on the result on our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
                // especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
                // Here we instead demonstrate using the clipper to only process lines that are within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
                // Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
                // both of which we can handle since we an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
                // Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char *line_start = buf + LineOffsets[line_no];
                        const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        ImGui::TextUnformatted(line_start, line_end);
                    }
                }
                clipper.End();
            }
        }

        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};

static void ImGui_Impl_Win32_UpdateMouseCursor()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
        return;
    }

    // Show OS mouse cursor
    LPTSTR win32_cursor = IDC_ARROW;
    switch (imgui_cursor)
    {
    case ImGuiMouseCursor_Arrow:
        win32_cursor = IDC_ARROW;
        break;
    case ImGuiMouseCursor_TextInput:
        win32_cursor = IDC_IBEAM;
        break;
    case ImGuiMouseCursor_ResizeAll:
        win32_cursor = IDC_SIZEALL;
        break;
    case ImGuiMouseCursor_ResizeEW:
        win32_cursor = IDC_SIZEWE;
        break;
    case ImGuiMouseCursor_ResizeNS:
        win32_cursor = IDC_SIZENS;
        break;
    case ImGuiMouseCursor_ResizeNESW:
        win32_cursor = IDC_SIZENESW;
        break;
    case ImGuiMouseCursor_ResizeNWSE:
        win32_cursor = IDC_SIZENWSE;
        break;
    case ImGuiMouseCursor_Hand:
        win32_cursor = IDC_HAND;
        break;
    case ImGuiMouseCursor_NotAllowed:
        win32_cursor = IDC_NO;
        break;
    }
    ::SetCursor(::LoadCursor(NULL, win32_cursor));
}

Gui::Gui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd)
    : m_dx12(new ImGuiDX12), m_logger(new ExampleAppLog)
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
    ImGui_Impl_ScreenState_Init();
    m_dx12->Initialize(device.Get(), bufferCount);

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

Gui::~Gui()
{
    ImGui::DestroyContext();
}

void Gui::BeginFrame(const screenstate::ScreenState &state)
{
    // Start the Dear ImGui frame
    ImGui_Impl_ScreenState_NewFrame(state);
    if (state.Has(screenstate::MouseButtonFlags::CursorUpdate))
    {
        ImGui_Impl_Win32_UpdateMouseCursor();
        // Update OS mouse cursor with the cursor requested by imgui
        // ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        // if (g_LastMouseCursor != mouse_cursor)
        // {
        //     g_LastMouseCursor = mouse_cursor;
        //     UpdateMouseCursor();
        // }
    }

    ImGui::NewFrame();
}

void Gui::EndFrame(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    ImGui::Render();
    m_dx12->RenderDrawData(commandList.Get(), ImGui::GetDrawData());
}

void Gui::Log(const char *msg)
{
    m_logger->AddLog(msg);
}

void Gui::ShowLogger()
{
    m_logger->Draw("Logger");
}

void Gui::Update(hierarchy::Scene *scene, float clearColor[4])
{
    //
    // imgui
    //
    {
        ImGui::Begin("main");
        if (ImGui::Button("open"))
        {
            auto path = OpenFileDialog(L"");
            auto node = hierarchy::SceneGltf::LoadFromPath(path);
            if (node)
            {
                LOGI << "load: " << path;
                scene->AddRootNode(node);
            }
            else
            {
                LOGW << "fail to load: " << path;
            }
        }
        ImGui::End();
    }
    {
        ShowLogger();
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
        ImGui::ColorEdit3("clear color", clearColor); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
}

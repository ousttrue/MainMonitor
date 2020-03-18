#include "Renderer.h"
#include "VR.h"
#include "save_windowplacement.h"
#include <Win32Window.h>
#include <hierarchy.h>
#include <d3d12.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

static std::shared_ptr<hierarchy::SceneMesh> CreateGrid()
{
    struct GridVertex
    {
        std::array<float, 2> position;
        std::array<float, 2> uv;
    };
    GridVertex vertices[] = {
        {{-1, 1}, {0, 0}},
        {{-1, -1}, {0, 1}},
        {{1, -1}, {1, 1}},
        {{1, 1}, {1, 0}},
    };
    uint16_t indices[] = {
        0, 1, 2, //
        2, 3, 0, //
    };
    auto mesh = hierarchy::SceneMesh::Create();
    mesh->vertices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Vertex,
        sizeof(vertices[0]), vertices, sizeof(vertices));
    mesh->indices = hierarchy::VertexBuffer::CreateStatic(
        hierarchy::Semantics::Index,
        2, indices, sizeof(indices));
    {
        auto material = hierarchy::SceneMaterial::Create();
        material->shader = hierarchy::ShaderManager::Instance().get("grid");
        mesh->submeshes.push_back({.draw_count = _countof(indices),
                                   .material = material});
    }
    return mesh;
}

const auto CLASS_NAME = L"ExternalViewerClass";
const auto WINDOW_NAME = L"ExternalViewer";

namespace plog
{
class MyFormatter
{
public:
    static util::nstring header()
    {
        return util::nstring();
    }

    static util::nstring format(const Record &r)
    {
        tm t;
        util::localtime_s(&t, &r.getTime().time);

        util::nostringstream ss;
        ss
            << std::setw(2) << t.tm_hour << ':' << std::setw(2) << t.tm_min << '.' << std::setw(2) << t.tm_sec
            << '[' << severityToString(r.getSeverity()) << ']'
            << r.getFunc() << '(' << r.getLine() << ") "
            << r.getMessage()

            << "\n"; // Produce a simple string with a log message.

        return ss.str();
    }
};

template <class Formatter>          // Typically a formatter is passed as a template parameter.
class MyAppender : public IAppender // All appenders MUST inherit IAppender interface.
{
    using OnWrite = std::function<void(const char *)>;
    OnWrite m_onWrite;

public:
    void write(const Record &record) override // This is a method from IAppender that MUST be implemented.
    {
        util::nstring str = Formatter::format(record); // Use the formatter to get a string from a record.
        if (m_onWrite)
        {
            auto utf8 = UTF8Converter::convert(str);
            m_onWrite(utf8.c_str());
        }
    }

    void onWrite(const OnWrite &callback)
    {
        m_onWrite = callback;
    }
};

} // namespace plog

int main(int argc, char **argv)
{
    static plog::ColorConsoleAppender<plog::MyFormatter> consoleAppender;

    static plog::MyAppender<plog::MyFormatter> imGuiAppender;
    Renderer renderer(65);
    imGuiAppender.onWrite(std::bind(&Renderer::log, &renderer, std::placeholders::_1));

    plog::init(plog::debug, &consoleAppender).addAppender(&imGuiAppender);

    auto path = std::filesystem::current_path();
    if (argc > 1)
    {
        path = argv[1];
    }
    hierarchy::ShaderManager::Instance().watch(path);

    screenstate::Win32Window window(CLASS_NAME);
    auto hwnd = window.Create(WINDOW_NAME);
    if (!hwnd)
    {
        LOGE << "fail to window.Create";
        return 1;
    }

    auto windowconf = std::filesystem::current_path().append("ExternalViewer.window.json").u16string();
    windowplacement::Restore(hwnd, SW_SHOW, (const wchar_t *)windowconf.c_str());
    window.OnDestroy = [hwnd, conf = windowconf]() {
        windowplacement::Save(hwnd, (const wchar_t *)conf.c_str());
    };
    // window.Show();

    {
        VR vr;

        if (vr.Connect())
        {
            LOGI << "vr.Connect";
        }
        else
        {
            LOGW << "fail to vr.Connect";
        }

        auto scene = renderer.GetScene();

        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(CreateGrid());
        scene->AddRootNode(node);

        if (argc > 2)
        {
            auto node = hierarchy::SceneGltf::LoadFromPath(argv[2]);
            if (node)
            {
                scene->AddRootNode(node);
                LOGI << "load: " << argv[2];
            }
            else
            {
                LOGW << "fail to load: " << argv[2];
            }
        }

        screenstate::ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame(scene);
            renderer.OnFrame(hwnd, state);
        }
    }

    LOGI << "exit";

    return 0;
}

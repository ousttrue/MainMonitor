#include "Renderer.h"
#include "VR.h"
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
    mesh->SetVertices(hierarchy::Semantics::Interleaved, sizeof(vertices[0]), vertices, sizeof(vertices));
    mesh->SetIndices(2, indices, sizeof(indices));
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

} // namespace plog

int main(int argc, char **argv)
{
    static plog::ColorConsoleAppender<plog::MyFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

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

    window.Show();

    {
        Renderer renderer(65);
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
        auto node = scene->CreateNode("grid");
        node->AddMesh(CreateGrid());

        if (argc > 2)
        {
            scene->LoadFromPath(argv[2]);
            LOGI << "load: " << argv[2];
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

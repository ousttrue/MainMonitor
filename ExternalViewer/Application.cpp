#include "Application.h"
#include "VR.h"
#include "CameraView.h"
#include "Gui/Gui.h"
#include "Gui/GuiView.h"
#include "Gizmo.h"
#include "frame_metrics.h"
#include <OrbitCamera.h>
#include "Renderer.h"
#include <hierarchy.h>
#include <functional>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

///
/// logger setup
///
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

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    VR m_vr;
    hierarchy::Scene m_scene;

    Renderer m_renderer;

    gui::Gui m_imgui;
    CameraView m_view;
    gizmesh::GizmoSystem::Buffer m_gizmoBuffer;
    std::shared_ptr<hierarchy::SceneView> m_sceneView;
    size_t m_viewTextureID = 0;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_renderer(256), m_sceneView(new hierarchy::SceneView)
    {
        m_sceneView->ClearColor = {
            0.3f,
            0.6f,
            0.5f,
            1.0f,
        };

        m_imGuiAppender.onWrite(std::bind(&gui::Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);

        auto path = std::filesystem::current_path();
        if (argc > 1)
        {
            path = argv[1];
        }
        hierarchy::ShaderManager::Instance().watch(path);

        {
            auto node = hierarchy::SceneNode::Create("grid");
            node->Mesh(hierarchy::CreateGrid());
            m_scene.gizmoNodes.push_back(node);
        }

        if (argc > 2)
        {
            auto model = hierarchy::SceneModel::LoadFromPath(argv[2]);
            if (model)
            {
                m_scene.sceneNodes.push_back(model->root);
            }
        }

        if (m_vr.Connect())
        {
            LOGI << "vr.Connect";
        }
        else
        {
            LOGW << "fail to vr.Connect";
        }
    }

    void OnFrame(void *hwnd, const screenstate::ScreenState &state)
    {
        if (!m_initialized)
        {
            m_renderer.Initialize(hwnd);
            m_initialized = true;
        }

        {
            frame_metrics::scoped s("vr");
            m_vr.OnFrame(&m_scene);
        }

        // imgui
        bool isShowView = false;
        screenstate::ScreenState viewState;
        {
            frame_metrics::scoped s("imgui");
            m_imgui.OnFrame(state, &m_scene);

            // view
            auto viewTextureID = m_renderer.ViewTextureID(m_sceneView);
            // imgui window for rendertarget. convert screenState for view
            isShowView = m_imgui.View(m_sceneView.get(), state, viewTextureID,
                                      &viewState);
        }

        // renderering
        {
            frame_metrics::scoped s("render");
            m_renderer.BeginFrame(hwnd, state.Width, state.Height);
            if (isShowView)
            {
                frame_metrics::scoped ss("view");
                m_view.Update3DView(viewState, m_scene.selected.lock());
                m_sceneView->Width = viewState.Width;
                m_sceneView->Height = viewState.Height;
                m_sceneView->Projection = m_view.Camera()->state.projection;
                m_sceneView->View = m_view.Camera()->state.view;
                m_sceneView->CameraPosition = m_view.Camera()->state.position;
                m_sceneView->CameraFovYRadians = m_view.Camera()->state.fovYRadians;
                UpdateDrawList();
                m_renderer.View(m_sceneView);
            }
            m_renderer.EndFrame();
        }
    }

    void UpdateDrawList()
    {
        m_sceneView->UpdateDrawList(&m_scene);

        // gizmo
        if (m_sceneView->ShowGizmo)
        {
            m_sceneView->Drawlist.Nodes.push_back({.NodeID = m_view.GizmoNodeID(),
                                        .WorldMatrix = {
                                            1, 0, 0, 0, //
                                            0, 1, 0, 0, //
                                            0, 0, 1, 0, //
                                            0, 0, 0, 1, //
                                        }});
            m_gizmoBuffer = m_view.GizmoBuffer();
            m_sceneView->Drawlist.Meshes.push_back({
                .NodeID = m_view.GizmoNodeID(),
                .Mesh = m_view.GizmoMesh(),
                .Vertices = {
                    .Ptr = m_gizmoBuffer.pVertices,
                    .Bytes = m_gizmoBuffer.verticesBytes,
                    .Stride = m_gizmoBuffer.vertexStride,
                },
                .Indices = {
                    .Ptr = m_gizmoBuffer.pIndices,
                    .Bytes = m_gizmoBuffer.indicesBytes,
                    .Stride = m_gizmoBuffer.indexStride,
                },
            });
        }
    }
};

Application::Application(int argc, char **argv)
    : m_impl(new ApplicationImpl(argc, argv))
{
}

Application::~Application()
{
    delete m_impl;
}

void Application::OnFrame(void *hwnd, const screenstate::ScreenState &state)
{
    m_impl->OnFrame(hwnd, state);
}

#include "Application.h"
#include "VR.h"
#include "Gui.h"
#include "Gizmo.h"
#include <OrbitCamera.h>
#include "Renderer.h"
#include <hierarchy.h>
#include <functional>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#define YAP_ENABLE
#include <YAP.h>

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

class View
{
    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    float clearColor[4] = {
        0.2f,
        0.2f,
        0.3f,
        1.0f};

    View()
    {
        m_camera.zNear = 0.01f;
    }

    const OrbitCamera *Camera() const
    {
        return &m_camera;
    }

    int GizmoNodeID() const
    {
        return m_gizmo.GetNodeID();
    }

    hierarchy::SceneMeshPtr GizmoMesh() const
    {
        return m_gizmo.GetMesh();
    }

    gizmesh::GizmoSystem::Buffer GizmoBuffer()
    {
        return m_gizmo.End();
    }

    void Update3DView(const screenstate::ScreenState &viewState, const hierarchy::SceneNodePtr &selected)
    {
        //
        // update camera
        //
        if (selected != m_selected)
        {
            if (selected)
            {
                m_camera.gaze = -selected->World().translation;
            }
            else
            {
                // m_camera->gaze = {0, 0, 0};
            }

            m_selected = selected;
        }
        m_camera.Update(viewState);

        //
        // update gizmo
        //
        m_gizmo.Begin(viewState, m_camera.state);
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
    }
};

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    VR m_vr;
    hierarchy::Scene m_scene;
    hierarchy::DrawList m_drawlist;

    Renderer m_renderer;

    Gui m_imgui;
    View m_view;
    gizmesh::GizmoSystem::Buffer m_gizmoBuffer;
    std::shared_ptr<hierarchy::SceneView> m_sceneView;
    size_t m_viewTextureID = 0;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_renderer(256), m_sceneView(new hierarchy::SceneView)
    {
        m_imGuiAppender.onWrite(std::bind(&Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);

        auto path = std::filesystem::current_path();
        if (argc > 1)
        {
            path = argv[1];
        }
        hierarchy::ShaderManager::Instance().watch(path);

        if (argc > 2)
        {
            auto node = hierarchy::SceneGltf::LoadFromPath(argv[2]);
            if (node)
            {
                m_scene.AddRootNode(node);
                LOGI << "load: " << argv[2];
            }
            else
            {
                LOGW << "fail to load: " << argv[2];
            }
        }

        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(CreateGrid());
        m_scene.AddRootNode(node);

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
            YAP::ScopedSection(VR);
            m_vr.OnFrame(&m_scene);
        }

        // imgui
        {
            YAP::ScopedSection(ImGui);
            m_imgui.NewFrame(state);
            m_imgui.Update(&m_scene, m_view.clearColor);
        }

        // renderering
        {
            YAP::ScopedSection(Render);
            m_renderer.BeginFrame(hwnd, state.Width, state.Height);

            auto viewTextureID = m_renderer.ViewTextureID(m_sceneView);
            screenstate::ScreenState viewState;
            bool isShowView = m_imgui.View(state, viewTextureID, &viewState);
            if (isShowView)
            {
                YAP::ScopedSection(View);
                m_view.Update3DView(viewState, m_imgui.Selected());
                m_sceneView->Width = viewState.Width;
                m_sceneView->Height = viewState.Height;
                m_sceneView->Projection = m_view.Camera()->state.projection;
                m_sceneView->View = m_view.Camera()->state.view;
                m_sceneView->CameraPosition = m_view.Camera()->state.position;
                m_sceneView->CameraFovYRadians = m_view.Camera()->state.fovYRadians;
                updateDrawList();
                m_renderer.View(m_sceneView, m_drawlist);
            }

            m_renderer.EndFrame();
        }
    }

    void updateDrawList()
    {
        m_drawlist.Clear();
        int rootCount;
        auto roots = m_scene.GetRootNodes(&rootCount);
        for (int i = 0; i < rootCount; ++i)
        {
            auto root = roots[i];
            root->UpdateWorld();
            m_drawlist.Traverse(root);
        }

        // gizmo
        m_drawlist.Nodes.push_back({.NodeID = m_view.GizmoNodeID(),
                                    .WorldMatrix = {
                                        1, 0, 0, 0, //
                                        0, 1, 0, 0, //
                                        0, 0, 1, 0, //
                                        0, 0, 0, 1, //
                                    }});
        m_gizmoBuffer = m_view.GizmoBuffer();
        m_drawlist.Meshes.push_back({
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

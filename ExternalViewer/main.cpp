#include "Renderer.h"
#include "VR.h"
#include <Win32Window.h>
#include <hierarchy.h>
#include <d3d12.h>

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

int main(int argc, char **argv)
{
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
        return 1;
    }

    window.Show();

    {
        Renderer renderer(65);
        VR vr;

        if (!vr.Connect())
        {
            return 2;
        }

        auto scene = renderer.GetScene();

        auto grid = CreateGrid();
        scene->AddMeshNode(grid);

        if (argc > 2)
        {
            scene->LoadFromPath(argv[2]);
        }

        screenstate::ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame(scene);
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

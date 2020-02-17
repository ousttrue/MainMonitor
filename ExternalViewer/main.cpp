#include "Window.h"
#include "Renderer.h"
#include "VR.h"
#include "Scene.h"

int main()
{
    Window window;

    auto hwnd = window.Create(L"ExternalViewerClass", L"ExternalViewer");
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

        Scene scene(vr::k_unMaxTrackedDeviceCount);
        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame(&scene);
            renderer.OnFrame(hwnd, state,
                             scene.Data(), scene.Count());
        }
    }

    return 0;
}

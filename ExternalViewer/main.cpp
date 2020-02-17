#include "Window.h"
#include "Renderer.h"
#include "VR.h"

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

        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame();
            renderer.OnFrame(hwnd, state,
                             vr.m_trackers, (int)_countof(vr.m_trackers));
        }
    }

    return 0;
}

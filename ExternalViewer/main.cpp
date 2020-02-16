#include "Window.h"
#include "Renderer.h"

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
        Renderer renderer;
        ScreenState state;
        while (window.Update(&state))
        {
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

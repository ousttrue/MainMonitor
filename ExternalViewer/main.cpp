#include "Window.h"

int main()
{
    Window window;

    auto hwnd = window.Create(L"ExternalViewerClass", L"ExternalViewer");
    if(!hwnd)
    {
        return 1;
    }

    window.Show();

    ScreenState state;
    while(window.Update(&state))
    {
        Sleep(30);
    }

    return 0;
}

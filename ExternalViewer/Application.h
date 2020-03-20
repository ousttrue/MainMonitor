#pragma once
#include <ScreenState.h>

class Application
{
    class ApplicationImpl *m_impl = nullptr;

public:
    Application(int argc, char **argv);
    ~Application();
    void OnFrame(void *hwnd, const screenstate::ScreenState &state);
};

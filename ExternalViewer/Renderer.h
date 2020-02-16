#pragma once
#include "ScreenState.h"

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer();
    ~Renderer();
    void OnFrame(void *hwnd, const ScreenState &state);
};

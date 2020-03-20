#pragma once
#include <memory>
#include <d12util.h>

namespace screenstate
{
struct ScreenState;
}

namespace hierarchy
{
class Scene;
}

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void OnFrame(void *hwnd, const screenstate::ScreenState &state);
    hierarchy::Scene *GetScene();
    void Log(const char *msg);
};

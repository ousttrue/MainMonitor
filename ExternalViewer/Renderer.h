#pragma once
#include <memory>

namespace camera
{
struct CameraState;
}

namespace hierarchy
{
struct DrawList;
}

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void Initialize(void *hwnd);
    void OnFrame(void *hwnd, int width, int height, hierarchy::DrawList *scene);

    size_t ViewTextureID();
    void UpdateViewResource(int width, int height, const camera::CameraState &camera);
};

#pragma once
#include <memory>

namespace hierarchy
{
struct DrawList;
struct SceneView;
} // namespace hierarchy

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void Initialize(void *hwnd);

    void BeginFrame(void *hwnd, int width, int height);
    void EndFrame();

    size_t ViewTextureID(const std::shared_ptr<hierarchy::SceneView> &view);
    void View(const std::shared_ptr<hierarchy::SceneView> &view,
              const hierarchy::DrawList &drawlist,
              const float clear[4]);
};

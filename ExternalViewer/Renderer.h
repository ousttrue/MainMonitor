#pragma once
#include <memory>
#include <gizmesh.h>

namespace screenstate
{
struct ScreenState;
}

namespace hierarchy
{
class Scene;
class SceneMesh;
struct DrawList;
} // namespace hierarchy

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void Initialize(void *hwnd);
    void OnFrame(void *hwnd, const screenstate::ScreenState &state, hierarchy::DrawList *scene);

    size_t ViewTextureID();
    void UpdateViewResource(const screenstate::ScreenState &viewState, const struct OrbitCamera *camera);
};

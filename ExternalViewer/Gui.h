#pragma once
#include <hierarchy.h>
#include <ScreenState.h>

#include <d3d12.h>
#include <wrl/client.h>

#include <list>
#include <memory>
#include <mutex>

class Gui
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class ImGuiDX12> m_dx12;
    std::unique_ptr<struct ExampleAppLog> m_logger;

    // single selection
    std::weak_ptr<hierarchy::SceneNode> m_selected;

public:
    Gui(const ComPtr<ID3D12Device> &device, int bufferCount, HWND hwnd);
    ~Gui();
    size_t GetOrCreateTexture(ID3D12Device *device,
                              ID3D12Resource *resource);
    void Remove(ID3D12Resource *resource);

    void BeginFrame(const screenstate::ScreenState &state);
    bool View(const screenstate::ScreenState &state, size_t textureID, screenstate::ScreenState *viewState);
    bool Update(hierarchy::Scene *scene, float clearColor[4]);
    void EndFrame(const ComPtr<ID3D12GraphicsCommandList> &commandList);

    hierarchy::SceneNodePtr Selected() const
    {
        return m_selected.lock();
    }

    void Log(const char *msg);
    void ShowLogger();

private:
    void DrawNode(const hierarchy::SceneNodePtr &node, hierarchy::SceneNode *selected);
};

#include <memory>
#include <unordered_map>
#include <hierarchy.h>
#include <d3d12.h>
#include <wrl/client.h>

namespace d12u
{

class SceneMapper
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class Uploader> m_uploader;
    std::unordered_map<hierarchy::SceneMeshPtr, std::shared_ptr<class Mesh>> m_meshMap;
    std::unordered_map<hierarchy::SceneViewPtr, std::shared_ptr<class RenderTargetChain>> m_renderTargetMap;

public:
    SceneMapper();
    class Uploader *GetUploader() { return m_uploader.get(); }
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    std::shared_ptr<class Mesh> GetOrCreate(const ComPtr<ID3D12Device> &device,
                                            const hierarchy::SceneMeshPtr &model,
                                            class RootSignature *rootSignature);
    std::shared_ptr<class RenderTargetChain> GetOrCreate(const hierarchy::SceneViewPtr &view);
};

} // namespace d12u

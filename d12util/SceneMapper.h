#include <memory>
#include <unordered_map>
#include <Scene.h>
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
    std::unordered_map<hierarchy::SceneImagePtr, std::shared_ptr<class Texture>> m_textureMap;

public:
    SceneMapper();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    std::shared_ptr<class Mesh> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneMeshPtr &model);
    std::shared_ptr<class Texture> GetOrCreate(const ComPtr<ID3D12Device> &device, const hierarchy::SceneImagePtr &image);
};
} // namespace d12u

#include <memory>
#include <unordered_map>
#include <Model.h>
#include <d3d12.h>
#include <wrl/client.h>

namespace d12u
{
class SceneMapper
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class Uploader> m_uploader;
    std::unordered_map<std::shared_ptr<hierarchy::Model>, std::shared_ptr<class Mesh>> m_modelMeshMap;

public:
    SceneMapper();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    std::shared_ptr<class Mesh> GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<hierarchy::Model> &model);
};
} // namespace d12u

#include "Helper.h"
#include <memory>
#include <hierarchy.h>

namespace d12u
{

class Shader;
class Material : NonCopyable
{
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    std::shared_ptr<Shader> m_shader;
    int m_lastGeneration = -1;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device, const hierarchy::SceneMaterialPtr &material);
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const std::shared_ptr<Shader> &shader,
                    const hierarchy::SceneMaterialPtr &material);
    bool Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u

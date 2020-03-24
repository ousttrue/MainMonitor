#include "Helper.h"
#include <memory>

namespace d12u
{

class Shader;
class Material : NonCopyable
{
    ComPtr<ID3D12PipelineState> m_pipelineState;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const std::shared_ptr<Shader> &shader);
    bool Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u

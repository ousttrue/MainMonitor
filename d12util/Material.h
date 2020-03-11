#include "Helper.h"

namespace d12u
{

class Material : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12PipelineState> m_pipelineState;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const std::string &shaderSource, UINT cbvDescriptorCount);
    void Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u

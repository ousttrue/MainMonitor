#pragma once
#include "Helper.h"
#include <list>
#include <functional>

namespace d12u
{
using OnCompletedFunc = std::function<void()>;

class CommandList : NonCopyable
{
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    std::list<OnCompletedFunc> m_callbacks;

public:
    const ComPtr<ID3D12GraphicsCommandList> &Get() const { return m_commandList; }

    void Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12PipelineState> &ps,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    void Reset(const ComPtr<ID3D12PipelineState> &ps);
    std::list<OnCompletedFunc> Close();
    void AddOnCompleted(const OnCompletedFunc &callback)
    {
        m_callbacks.push_back(callback);
    }
};
} // namespace d12u

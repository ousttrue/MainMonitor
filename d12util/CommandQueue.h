#pragma once
#include "Helper.h"
#include <list>
#include <functional>

namespace d12u
{
class CommandQueue : NonCopyable
{
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_nextFenceValue = 0;
    HANDLE m_fenceEvent = NULL;

public:
    using CallbackList = std::list<std::function<void()>>;
    const ComPtr<ID3D12CommandQueue> &Get() const { return m_queue; }

    ~CommandQueue();
    void Initialize(const ComPtr<ID3D12Device> &device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    void Execute(const ComPtr<ID3D12CommandList> &commandList);
    UINT64 Signal();
    UINT64 CurrentValue() const;
    void SyncFence(const CallbackList &callbacks = CallbackList());
};

} // namespace d12u
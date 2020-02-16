#include "CommandQueue.h"

namespace d12u
{

CommandQueue::~CommandQueue()
{
    SyncFence();
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
    }
}

void CommandQueue::Initialize(const ComPtr<ID3D12Device> &device,
                              D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {
        .Type = type,
    };
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_nextFenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void CommandQueue::Execute(const ComPtr<ID3D12CommandList> &commandList)
{
    ID3D12CommandList *list[] = {
        commandList.Get(),
    };
    m_queue->ExecuteCommandLists(_countof(list), list);
}

UINT64 CommandQueue::Signal()
{
    // Signal and increment the fence value.
    const UINT64 fence = m_nextFenceValue++;
    ThrowIfFailed(m_queue->Signal(m_fence.Get(), fence));
    return fence;
}

UINT64 CommandQueue::CurrentValue() const
{
    return m_fence->GetCompletedValue();
}

void CommandQueue::SyncFence(const CallbackList &callbacks)
{
    auto fence = Signal();

    // Wait until the previous frame is finished.
    if (CurrentValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    for (auto &callback : callbacks)
    {
        callback();
    }
}

} // namespace d12u
#include "Uploader.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ResourceItem.h"

namespace d12u
{
Uploader::Uploader()
    : m_queue(new CommandQueue), m_commandList(new CommandList)
{
}

Uploader::~Uploader()
{
    m_queue->SyncFence();
    delete m_commandList;
    delete m_queue;
}

void Uploader::Initialize(const ComPtr<ID3D12Device> &device)
{
    m_queue->Initialize(device, D3D12_COMMAND_LIST_TYPE_COPY);
    m_commandList->Initialize(device, nullptr, D3D12_COMMAND_LIST_TYPE_COPY);
}

void Uploader::Update(const ComPtr<ID3D12Device> &device)
{
    if (!m_callback)
    {
        if (m_commands.empty())
        {
            return;
        }

        // dequeue -> execute
        auto command = m_commands.front();
        m_commands.pop();

        if (m_upload)
        {
            auto desc = m_upload->Resource()->GetDesc();
            if (desc.Width * desc.Height < command.ByteLength)
            {
                // clear
                m_upload.reset();
            }
        }
        if (!m_upload)
        {
            m_upload = ResourceItem::CreateUpload(device, command.ByteLength);
        }

        m_commandList->Reset(nullptr);
        command.Item->EnqueueUpload(m_commandList, m_upload, command.Data, command.ByteLength, command.Stride);
        auto callbacks = m_commandList->Close();
        m_queue->Execute(m_commandList->Get());
        m_callbackFenceValue = m_queue->Signal();
        m_callback = [callbacks]() {
            for (auto &callback : callbacks)
            {
                callback();
            }
        };
    }
    else
    {
        // wait fence
        if (m_callbackFenceValue <= m_queue->CurrentValue())
        {
            // callback is done
            m_callback();
            // clear
            m_callback = OnCompletedFunc();
        }
    }
}
} // namespace d12u
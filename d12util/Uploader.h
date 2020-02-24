#pragma
#include "Helper.h"
#include <queue>
#include <functional>

namespace d12u
{
struct UploadCommand
{
    std::shared_ptr<class ResourceItem> Item;
    const void *Data = nullptr;
    UINT ByteLength = 0;
    UINT Stride = 0;
    std::vector<uint8_t> Payload;

    UploadCommand(const UploadCommand &rhs) = delete;
    UploadCommand &operator=(const UploadCommand &rhs) = delete;
    UploadCommand() {}
    UploadCommand(const std::shared_ptr<class ResourceItem> &item,
                  const void *data, UINT byteLength, UINT stride)
        : Item(item), Data(data), ByteLength(byteLength), Stride(stride)
    {
    }

    void UsePayload(const std::shared_ptr<class ResourceItem> &item, UINT stride)
    {
        Item = item;
        Stride = stride;
        // refer payload. payload has lifetime while MapCopyUnmap
        Data = Payload.data();
        ByteLength = (UINT)Payload.size();
    }
};

class Uploader : NonCopyable
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class CommandQueue *m_queue = nullptr;
    class CommandList *m_commandList = nullptr;

    std::queue<std::shared_ptr<UploadCommand>> m_commands;
    using OnCompletedFunc = std::function<void()>;
    OnCompletedFunc m_callback;
    UINT64 m_callbackFenceValue = 0;

    // for upload staging
    std::shared_ptr<class ResourceItem> m_upload;

public:
    Uploader();
    ~Uploader();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    void EnqueueUpload(const std::shared_ptr<class ResourceItem> &item,
                       const void *p, UINT byteLength, UINT stride)
    {
        auto command = std::make_shared<UploadCommand>(item, p, byteLength, stride);
        EnqueueUpload(command);
    }
    void EnqueueUpload(const std::shared_ptr<UploadCommand> &command)
    {
        m_commands.push(command);
    }
};
} // namespace d12u
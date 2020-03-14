#pragma once
#include "Helper.h"
#include <memory>

namespace d12u
{

class Texture : NonCopyable
{
public:
    std::shared_ptr<class ResourceItem> m_imageBuffer;

    const Microsoft::WRL::ComPtr<ID3D12Resource> &Resource() const;

    void ImageBuffer(const std::shared_ptr<class ResourceItem> &item)
    {
        m_imageBuffer = item;
    }

    bool IsDrawable(class CommandList *commandList, UINT rootParameterIndex);
};

} // namespace d12u
#include "Texture.h"
#include "CommandList.h"
#include "ResourceItem.h"

namespace d12u
{

const Microsoft::WRL::ComPtr<ID3D12Resource> &Texture::Resource() const
{
    return m_imageBuffer->Resource();
}

bool Texture::IsDrawable(class CommandList *commandList, UINT rootParameterIndex)
{
    auto _commandList = commandList->Get();

    if (!m_imageBuffer)
    {
        return false;
    }

    auto state = m_imageBuffer->State();
    if (state.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (state.Upload == UploadStates::Uploaded)
        {
            m_imageBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    }

    if (!state.Drawable())
    {
        return false;
    }

    return true;
}

} // namespace d12u
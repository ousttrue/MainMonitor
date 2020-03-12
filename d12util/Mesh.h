#pragma once
#include "Helper.h"
#include <memory>
#include <vector>

namespace d12u
{

class Mesh : NonCopyable
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::shared_ptr<class ResourceItem> m_vertexBuffer;
    std::shared_ptr<class ResourceItem> m_indexBuffer;

public:
    void VertexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_vertexBuffer = item; }
    const std::shared_ptr<class ResourceItem> &VertexBuffer() const { return m_vertexBuffer; }
    void IndexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_indexBuffer = item; }
    const std::shared_ptr<class ResourceItem> &IndexBuffer() const { return m_indexBuffer; }
    bool IsDrawable(class CommandList *commandList);
};

} // namespace d12u

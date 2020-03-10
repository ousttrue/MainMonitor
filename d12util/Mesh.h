#pragma once
#include "Helper.h"
#include <memory>

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
    void Command(class CommandList *commandList)
    {
        if (m_indexBuffer && m_vertexBuffer)
        {
            Mesh::IndexedCommand(commandList);
        }
        else if (m_vertexBuffer)
        {
            Mesh::NonIndexedCommand(commandList);
        }
    }

private:
    void IndexedCommand(class CommandList *commandList);
    void NonIndexedCommand(class CommandList *commandList);
};
} // namespace d12u
#include "VertexBuffer.h"

namespace hierarchy
{

void VertexBuffer::Append(const std::shared_ptr<VertexBuffer> &vb)
{
    if (semantic != vb->semantic)
    {
        throw;
    }
    if (stride != vb->stride)
    {
        throw;
    }
    buffer.reserve(buffer.size() + vb->buffer.size());
    std::copy(vb->buffer.begin(), vb->buffer.end(), std::back_inserter(buffer));
}

} // namespace hierarchy
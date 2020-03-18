#pragma once
#include <vector>
#include <memory>
#include <stdint.h>

namespace hierarchy
{

enum class Semantics
{
    Unknown,
    Vertex,
    Index,
};

class VertexBuffer
{
public:
    Semantics semantic{};
    uint32_t stride{};
    bool isDynamic{};
    std::vector<uint8_t> buffer;

    // dynamic
    static std::shared_ptr<VertexBuffer> CreateDynamic(Semantics semantic, uint32_t stride, uint32_t size)
    {
        auto vb = std::make_shared<VertexBuffer>();
        vb->semantic = semantic;
        vb->stride = stride;
        vb->isDynamic = true;
        vb->buffer.resize(size);
        return vb;
    }

    // static. use payload
    static std::shared_ptr<VertexBuffer> CreateStatic(Semantics semantic, uint32_t stride, const void *_p, uint32_t size)
    {
        auto vb = std::make_shared<VertexBuffer>();
        vb->semantic = semantic;
        vb->stride = stride;
        vb->isDynamic = false;
        auto p = (const uint8_t *)_p;
        vb->buffer.assign(p, p + size);
        return vb;
    }

    uint32_t Count() const { return (uint32_t)buffer.size() / stride; }
    void Append(const std::shared_ptr<VertexBuffer> &buffer);
};

} // namespace hierarchy

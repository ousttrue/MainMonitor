#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <ranges>
#include "SceneMaterial.h"

namespace hierarchy
{

enum class Semantics
{
    Vertex,
    Index,
};

struct VertexBuffer
{
    Semantics semantic;
    uint32_t stride;
    bool isDynamic;
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

struct SceneSubmesh
{
    uint32_t draw_count = 0;
    SceneMaterialPtr material;
};

class SceneNode;
class SceneMeshSkin
{
public:
    std::vector<std::shared_ptr<SceneNode>> joints;
    std::vector<std::array<float, 16>> inverseBindMatrices;
    std::vector<std::array<float, 16>> skinningMatrices;
    std::vector<uint8_t> cpuSkiningBuffer;
    void Update(const void *vertices, uint8_t stride, uint8_t vertexCount);
};
using SceneMeshSkinPtr = std::shared_ptr<SceneMeshSkin>;

class SceneMesh
{
public:
    static std::shared_ptr<SceneMesh> Create();
    static std::shared_ptr<SceneMesh> CreateDynamic(
        uint32_t vertexReserve, uint32_t vertexStride,
        uint32_t indexReserve, uint32_t indexStride);

    std::wstring name;

    std::shared_ptr<VertexBuffer> vertices;
    std::shared_ptr<VertexBuffer> indices;

    std::vector<SceneSubmesh> submeshes;
    void AddSubmesh(const std::shared_ptr<SceneMesh> &mesh);

    SceneMeshSkinPtr skin;
};
using SceneMeshPtr = std::shared_ptr<SceneMesh>;

} // namespace hierarchy

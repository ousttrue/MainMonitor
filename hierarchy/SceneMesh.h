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
    Interleaved,
    Index,
    Position,
    Normal,
    TexCoord,
};

struct VertexBuffer
{
    Semantics semantic;
    std::vector<uint8_t> buffer;
    uint32_t stride;
    bool isDynamic = false;
    uint32_t Count() const { return (uint32_t)buffer.size() / stride; }
    void Append(const VertexBuffer &buffer);
};

struct SceneSubmesh
{
    // uint32_t draw_offset = 0;
    uint32_t draw_count = 0;
    SceneMaterialPtr material;
};

class SceneMesh
{
    std::vector<VertexBuffer> m_vertices;
    VertexBuffer m_indices;

public:
    static std::shared_ptr<SceneMesh> Create();
    static std::shared_ptr<SceneMesh> CreateDynamic(int vertexReserve, int indexReserve);

    std::vector<SceneSubmesh> submeshes;
    void AddSubmesh(const std::shared_ptr<SceneMesh> &mesh);

    std::wstring name;

    void SetVertices(Semantics semantic, uint32_t stride, const void *p, uint32_t size);
    void SetVertices(const VertexBuffer &vertices)
    {
        m_vertices.push_back(vertices);
    }
    const VertexBuffer *GetVertices(Semantics semantic);

    void SetIndices(uint32_t stride, const void *indices, uint32_t size);
    const VertexBuffer *GetIndices() const { return &m_indices; }
};
using SceneMeshPtr = std::shared_ptr<SceneMesh>;

} // namespace hierarchy

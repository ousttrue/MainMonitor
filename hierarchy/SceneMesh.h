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
    Index,
    Position,
    Normal,
    TexCoord,
    PositionNormalTexCoord,
    PositionNormalColor,
};

enum class ValueType
{
    UInt16 = 2,
    UInt32 = 4,
    Float2 = 8,
    Float3 = 12,
    Float4 = 16,
    Float8 = 32,  // Position, Normal, TexCoord
    Float10 = 40, // Position, Normal, Color
};

struct VertexBuffer
{
    Semantics semantic;
    std::vector<uint8_t> buffer;
    ValueType valueType;
    bool isDynamic = false;
    uint32_t Stride() const { return (uint32_t)valueType; }
    uint32_t Count() const { return (uint32_t)buffer.size() / Stride(); }
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

    void SetVertices(Semantics semantic, ValueType valueType, const void *p, uint32_t size);
    void SetVertices(const VertexBuffer &vertices)
    {
        m_vertices.push_back(vertices);
    }
    const VertexBuffer *GetVertices(Semantics semantic);

    void SetIndices(ValueType valueType, const void *indices, uint32_t size);
    const VertexBuffer *GetIndices() const { return &m_indices; }
};
using SceneMeshPtr = std::shared_ptr<SceneMesh>;

} // namespace hierarchy

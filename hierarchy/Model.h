#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
// #include "ConstantBuffer.h"
// #include "d12util.h"

namespace hierarchy
{
class Model
{
    int m_id = -1;

    std::vector<uint8_t> m_vertices;
    int m_vertexStride = 0;
    std::vector<uint8_t> m_indices;
    int m_indexStride = 0;

    Model(int id)
        : m_id(id)
    {
    }

public:
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 world{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    };
    ModelConstantBuffer Data;
    static std::shared_ptr<Model> Create();

    int ID() const { return m_id; }
    void SetVertices(const uint8_t *p, int byteLength, int stride);
    template <typename T, size_t N>
    void SetVertices(T (&values)[N])
    {
        SetVertices((const uint8_t *)values, sizeof(values), sizeof(T));
    }
    const uint8_t *Vertices() const { return m_vertices.data(); }
    uint32_t VerticesByteLength() const { return (uint32_t)m_vertices.size(); }
    int VertexStride() const { return m_vertexStride; }

    void SetIndices(const uint8_t *p, int byteLength, int stride);
    template <typename T, size_t N>
    void SetIndices(T (&values)[N])
    {
        SetIndices((const uint8_t *)values, sizeof(values), sizeof(T));
    }
    const uint8_t *Indices() const { return m_indices.data(); }
    uint32_t IndicesByteLength() const { return (uint32_t)m_indices.size(); }
    int IndexStride() const { return m_indexStride; }
};
} // namespace hierarchy
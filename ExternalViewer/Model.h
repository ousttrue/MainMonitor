#pragma once
#include <vector>
#include <memory>
#include <stdint.h>

class Model
{
    std::vector<uint8_t> m_vertices;
    int m_vertexStride = 0;
    std::vector<uint8_t> m_indices;
    int m_indexStride = 0;

public:
    static std::shared_ptr<Model> Create();

    void SetVeritces(const uint8_t *p, int byteLength, int stride);
    void SetIndices(const uint8_t *p, int byteLength, int stride);
};

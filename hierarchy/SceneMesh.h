#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <ranges>
#include "SceneMaterial.h"

namespace hierarchy
{

struct SceneSubmesh
{
    uint32_t draw_count = 0;
    SceneMaterialPtr material;
};

class SceneMesh
{
public:
    static std::shared_ptr<SceneMesh> Create();
    static std::shared_ptr<SceneMesh> CreateDynamic(
        uint32_t vertexReserve, uint32_t vertexStride,
        uint32_t indexReserve, uint32_t indexStride);

    std::wstring name;

    std::shared_ptr<class VertexBuffer> vertices;
    std::shared_ptr<class VertexBuffer> indices;

    std::vector<SceneSubmesh> submeshes;
    void AddSubmesh(const std::shared_ptr<SceneMesh> &mesh);

    std::shared_ptr<class SceneMeshSkin> skin;
};
using SceneMeshPtr = std::shared_ptr<SceneMesh>;

} // namespace hierarchy

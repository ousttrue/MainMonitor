#pragma once
#include "ScreenState.h"
#include <memory>

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer();
    ~Renderer();
    void OnFrame(void *hwnd, const ScreenState &state,
                 const std::shared_ptr<class Model> *models, int count);
    void AddModel(int index,
                  const uint8_t *vertices, int verticesByteLength, int vertexStride,
                  const uint8_t *indices, int indicesByteLength, int indexStride);
};

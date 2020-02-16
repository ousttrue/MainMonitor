#pragma once
#include "ScreenState.h"

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer();
    ~Renderer();
    void OnFrame(void *hwnd, const ScreenState &state);
    void AddModel(int index,
                  const uint8_t *vertices, int verticesByteLength, int vertexStride,
                  const uint8_t *indices, int indicesByteLength, int indexStride);
};

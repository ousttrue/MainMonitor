#include "Window.h"
#include "Renderer.h"
#include "VR.h"

int main()
{
    Window window;

    auto hwnd = window.Create(L"ExternalViewerClass", L"ExternalViewer");
    if (!hwnd)
    {
        return 1;
    }

    window.Show();

    {
        Renderer renderer;
        VR vr;

        auto callback = [&renderer](int index,
                                    const vr::RenderModel_t *model,
                                    const vr::RenderModel_TextureMap_t *texture) {
        };

        vr.SetCallback(callback);

        if (!vr.Connect())
        {
            return 2;
        }

        ScreenState state;
        while (window.Update(&state))
        {
            vr.OnFrame();
            for (auto &kv : vr.m_trackers)
            {
                auto &index = kv.first;
                auto &tracker = kv.second;
                if (tracker->Update())
                {
                    // tracker model loaded
                    auto model = tracker->Model();
                    auto vertexStride = (int)sizeof(model->rVertexData[0]);
                    auto indexStride = (int)sizeof(model->rIndexData[0]);
                    renderer.AddModel(index,
                                      (uint8_t *)model->rVertexData, model->unVertexCount * vertexStride, vertexStride,
                                      (uint8_t *)model->rIndexData, model->unTriangleCount * indexStride, indexStride);
                }
                renderer.SetPose(index, &tracker->Matrix()._11);
            }

            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}

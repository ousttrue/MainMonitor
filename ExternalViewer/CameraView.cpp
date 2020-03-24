#include "CameraView.h"
#include <ScreenState.h>
#include <hierarchy.h>

void CameraView::Update3DView(const screenstate::ScreenState &viewState, const hierarchy::SceneNodePtr &selected)
{
    //
    // update camera
    //
    if (selected != m_selected)
    {
        if (selected)
        {
            m_camera.gaze = -selected->World().translation;
        }
        else
        {
            // m_camera->gaze = {0, 0, 0};
        }

        m_selected = selected;
    }
    m_camera.Update(viewState);

    //
    // update gizmo
    //
    m_gizmo.Begin(viewState, m_camera.state);
    if (selected)
    {
        // if (selected->EnableGizmo())
        {
            auto parent = selected->Parent();
            m_gizmo.Transform(selected->ID(),
                              selected->Local(),
                              parent ? parent->World() : falg::Transform{});
        }
    }
}

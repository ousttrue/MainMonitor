#pragma once
#include "Gizmo.h"
#include <OrbitCamera.h>
#include <hierarchy.h>

namespace gui
{

class View
{
    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    float clearColor[4] = {
        0.2f,
        0.2f,
        0.3f,
        1.0f};

    View()
    {
        m_camera.zNear = 0.01f;
    }

    const OrbitCamera *Camera() const
    {
        return &m_camera;
    }

    int GizmoNodeID() const
    {
        return m_gizmo.GetNodeID();
    }

    hierarchy::SceneMeshPtr GizmoMesh() const
    {
        return m_gizmo.GetMesh();
    }

    gizmesh::GizmoSystem::Buffer GizmoBuffer()
    {
        return m_gizmo.End();
    }

    void Update3DView(const screenstate::ScreenState &viewState, const hierarchy::SceneNodePtr &selected);
};

} // namespace gui

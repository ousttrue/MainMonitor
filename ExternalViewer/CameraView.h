#pragma once
#include "Gizmo.h"
#include <OrbitCamera.h>
#include <hierarchy.h>

class CameraView
{
    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    CameraView()
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

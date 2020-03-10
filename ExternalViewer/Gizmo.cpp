#include "Gizmo.h"
#include <gizmesh.h>
#include <SceneNode.h>

Gizmo::Gizmo()
    : m_gizmo(new gizmesh::GizmoSystem),
      m_gizmoMesh(hierarchy::SceneMesh::CreateDynamic(65535, 65535))
{
}

Gizmo::~Gizmo()
{
    delete m_gizmo;
}

void Gizmo::Begin(const screenstate::ScreenState &state, const camera::CameraState &camera)
{
    if (state.KeyCode['T'])
    {
        m_mode = GizmoModes::Translate;
    }
    if (state.KeyCode['R'])
    {
        m_mode = GizmoModes::Rotation;
    }
    if (state.KeyCode['S'])
    {
        m_mode = GizmoModes::Scale;
    }

    m_gizmo->begin(camera.position, camera.rotation,
                   camera.ray_origin, camera.ray_direction,
                   state.MouseLeftDown());
}

gizmesh::GizmoSystem::Buffer Gizmo::End()
{
    return m_gizmo->end();
}

void Gizmo::Transform(int id, fpalg::TRS &trs)
{
    switch (m_mode)
    {
    case GizmoModes::Translate:
        gizmesh::handle::translation(*m_gizmo, id, trs, true);
        break;
    case GizmoModes::Rotation:
        gizmesh::handle::rotation(*m_gizmo, id, trs, true);
        break;
    case GizmoModes::Scale:
        gizmesh::handle::scale(*m_gizmo, id, trs, true);
        break;
    default:
        throw;
        ;
    }
}

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

void Gizmo::Begin(const camera::CameraState &camera, bool button)
{
    m_gizmo->begin(camera.position, camera.rotation,
                   camera.ray_origin, camera.ray_direction,
                   button);
}

gizmesh::GizmoSystem::Buffer Gizmo::End()
{
    return m_gizmo->end();
}

void Gizmo::Transform(int id, fpalg::TRS &trs)
{
    gizmesh::handle::translation(*m_gizmo, id, trs, true);
}

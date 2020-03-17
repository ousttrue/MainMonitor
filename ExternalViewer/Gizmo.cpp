#include "Gizmo.h"
#include <gizmesh.h>
#include <hierarchy.h>

Gizmo::Gizmo()
    : m_gizmoMesh(hierarchy::SceneMesh::CreateDynamic(65535, 65535))
{
}

Gizmo::~Gizmo()
{
    delete m_gizmo;
}

void Gizmo::Begin(const screenstate::ScreenState &state, const camera::CameraState &camera)
{
    if (!m_gizmo)
    {
        m_gizmo = new gizmesh::GizmoSystem;
        auto material = hierarchy::SceneMaterial::Create();
        material->shader = hierarchy::ShaderManager::Instance().get("gizmo");
        m_gizmoMesh->submeshes.push_back({
            .material = material,
        });
    }

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
    if (state.KeyCode['\t'])
    {
        m_isLocal = !m_isLocal;
    }

    m_gizmo->begin(camera.position, camera.rotation,
                   camera.ray_origin, camera.ray_direction,
                   state.MouseLeftDown());
}

gizmesh::GizmoSystem::Buffer Gizmo::End()
{
    auto buffer = m_gizmo->end();
    m_gizmoMesh->submeshes.back().draw_count = buffer.indicesBytes / buffer.indexStride;
    return buffer;
}

void Gizmo::Transform(int id, falg::Transform &local, const falg::Transform &parent)
{
    // falg::TRS trs(transform.translation, transform.rotation, {1, 1, 1});
    switch (m_mode)
    {
    case GizmoModes::Translate:
        gizmesh::handle::translation(*m_gizmo, id, m_isLocal,
                                     &parent, local.translation, local.rotation);
        break;

    case GizmoModes::Rotation:
        gizmesh::handle::rotation(*m_gizmo, id, m_isLocal,
                                  &parent, local.translation, local.rotation);
        break;

    // case GizmoModes::Scale:
    //     gizmesh::handle::scale(*m_gizmo, id, trs, true);
    //     break;
    
    default:
        throw;
        ;
    }
    // transform = trs.transform;
}

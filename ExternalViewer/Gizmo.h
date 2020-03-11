#pragma
#include <gizmesh.h>
#include <SceneNode.h>
#include <ScreenState.h>
#include <CameraState.h>

enum class GizmoModes
{
    Translate,
    Rotation,
    Scale,
};

class Gizmo
{
    struct gizmesh::GizmoSystem *m_gizmo = nullptr;
    hierarchy::SceneNodePtr m_gizmoNode; // for node id
    hierarchy::SceneMeshPtr m_gizmoMesh;
    GizmoModes m_mode = GizmoModes::Translate;

public:
    Gizmo();
    ~Gizmo();

    int GetNodeID()
    {
        if (!m_gizmoNode)
        {
            m_gizmoNode = hierarchy::SceneNode::Create();
        }
        return m_gizmoNode->ID();
    }

    hierarchy::SceneMeshPtr GetMesh() const { return m_gizmoMesh; }

    void Begin(const screenstate::ScreenState &state, const camera::CameraState &camera);
    gizmesh::GizmoSystem::Buffer End();
    void Transform(int id, fpalg::TRS &trs);
};
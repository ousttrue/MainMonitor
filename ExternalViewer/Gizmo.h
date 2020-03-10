#pragma
#include <gizmesh.h>
#include <SceneNode.h>
#include <CameraState.h>

class Gizmo
{
    struct gizmesh::GizmoSystem *m_gizmo = nullptr;
    hierarchy::SceneNodePtr m_gizmoNode; // for node id
    hierarchy::SceneMeshPtr m_gizmoMesh;

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

    void Begin(const camera::CameraState &camera, bool button);
    gizmesh::GizmoSystem::Buffer End();
    void Transform(int id, fpalg::TRS &trs);
};

# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* [x] windows position restore
* shader/material
    * [ ] light/lambert
    * [x] Grid
    * [ ] glTF pbr
    * [ ] alpha blending
    * [ ] reflection Constants Semantics
* imgui
    * [x] frame rate
    * [x] clear color
    * [x] scene tree
    * [x] docking
    * [x] consume wheel, click event.
    * [x] select node
    * [ ] selected node transform(T, R, S)
    * [ ] camera info
    * [ ] light info
    * [ ] gizmo info
* gizmo
    * [x] VertexColor
    * [ ] Hover
    * [x] scene hierarchy(has parent)
    * [ ] undo
    * [ ] bone
* glTF
    * [x] node
    * [x] CPU skinning
    * [ ] GPU skinning
    * [ ] animation
* scene
    * [ ] light node
    * [ ] manage node id(d3d12 slot)
    * [ ] manage material id(d3d12 slot)
    * [ ] camera gaze to selected node
    * [ ] GRPC remote API
    * [ ] bullet physics

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse

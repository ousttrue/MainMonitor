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
    * [ ] profiler
    * [ ] node context menu(remove)
    * [ ] color log
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
    * [ ] counter matrix when without skin.skeleton
    * [ ] animation
* scene
    * [ ] light node
    * [ ] manage node id(d3d12 slot)
    * [ ] manage material id(d3d12 slot)
    * [ ] camera gaze to selected node
    * [ ] GRPC remote API
    * [ ] bullet physics
    * [ ] load on thread
        * [ ] progress bar
    * [ ] bvh load
    * [ ] node type

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse

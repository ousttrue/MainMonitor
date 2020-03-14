# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* [ ] windows position restore
* shader/material
    * [ ] light/lambert
    * [x] Grid
    * [ ] glTF pbr
    * [ ] alpha blending
    * [ ] reflection Constants Semantics
* imgui
    * [x] frame rate
    * [x] clear color
    * [ ] scene tree
    * [ ] docking
* gizmo
    * [x] VertexColor
    * [ ] Hover
    * [ ] local from parent
* glTF
    * [x] node
    * [ ] CPU skinning
    * [ ] GPU skinning
    * [ ] animation
* scene
    * [ ] select node
    * [ ] light node

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse

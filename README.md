# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* shader/material
    * [ ] light/lambert
    * [x] Grid
    * [ ] glTF pbr
    * [ ] alpha blending
    * [ ] reflection Constants Semantics
* imgui
    * [ ] frame rate
    * [ ] clear color
    * [ ] scene tree
* gizmo
    * [x] VertexColor
    * [ ] Hover
* glTF
    * [ ] node
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

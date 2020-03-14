# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* [ ] Grid
* [x] imgui
* [x] gizmo
    * [x] VertexColor
    * [ ] light transform
* [ ] light/lambert
* [x] material
* [x] texture
* [x] shader runtime load
    * [x] shader reload when hlsl updated
* [ ] logger
    * [ ] imgui log widgets
* [x] Shader Reflection
    * [ ] Constants Semantics
* [ ] glTF node
* [ ] glTF pbr
* [ ] select scene node

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse

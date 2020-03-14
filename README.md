# MainMonitor
VR desktop experiment

## desktop_dupl(OpenVR dashboard overlay)

* [x] replace CopyResource to Quad Render 
* [x] mouse cursor
* [ ] reference tracker position

## MainMonitor(Unity OpenVR App)

TODO

## ExternalViewer(OpenVR tracker viewer)

* [x] Grid
* [x] imgui
* [x] gizmo
    * [x] VertexColor
    * [ ] Hover
* [ ] light/lambert
* [x] material
    * [ ] alpha blending
* [x] texture
* [x] shader runtime load
    * [x] shader reload when hlsl updated
* [x] logger
    * [ ] imgui log widgets
* [x] Shader Reflection
    * [ ] Constants Semantics
* [ ] glTF node
* [ ] glTF pbr
* [ ] glTF animation
* scene
    * [ ] node name
    * [ ] select node
    * [ ] light node

## hlsl memo

VSInput vs;
PSInput ps;
constants
    scene: sProjection
    node: nModel
    material: mDiffuse

set(TARGET_NAME ExternalViewer)
add_executable(${TARGET_NAME}
    main.cpp
    Renderer.cpp
    VR.cpp
    Application.cpp
    Gui/ImGuiDX12.cpp
    Gui/ImGuiDX12FrameResources.cpp
    Gui/ImGuiImplScreenState.cpp
    Gui/Gui.cpp
    Gui/Gizmo.cpp
    Gui/View.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/headers
    ${EXTERNAL_DIR}/plog/include
    ${EXTERNAL_DIR}/nlohmann-json/include
    ${EXTERNAL_DIR}/IconFontCppHeaders
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    falg
    window_example
    gizmesh
    #
    d12util
    hierarchy
    #
    imgui
    ${EXTERNAL_DIR}/openvr/lib/win64/openvr_api.lib
    #
    d3dcompiler
    d3d12
    dxgi
    )

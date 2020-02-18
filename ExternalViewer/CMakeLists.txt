set(TARGET_NAME ExternalViewer)
add_executable(${TARGET_NAME}
    main.cpp
    Renderer.cpp
    Model.cpp
    Camera.cpp
    VR.cpp
    Scene.cpp
    ImGuiDX12.cpp
    ImGuiDX12FrameResources.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    d12util
    #
    imgui
    ${EXTERNAL_DIR}/openvr/lib/win64/openvr_api.lib
    #
    d3dcompiler
    Winmm
    d3d12
    dxgi
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/headers
    )

set(TARGET_NAME ExternalViewer)
add_executable(${TARGET_NAME}
    main.cpp
    Renderer.cpp
    VR.cpp
    ImGuiDX12.cpp
    ImGuiDX12FrameResources.cpp
    ImGuiImplScreenState.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/headers
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    fpalg
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
    Winmm
    d3d12
    dxgi
    )

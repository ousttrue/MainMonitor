set(TARGET_NAME ExternalViewer)
add_executable(${TARGET_NAME}
    main.cpp
    Renderer.cpp
    Model.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    Winmm
    d3d12
    dxgi
    d12util
    d3dcompiler
    ${EXTERNAL_DIR}/openvr/lib/win64/openvr_api.lib
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/headers
    )

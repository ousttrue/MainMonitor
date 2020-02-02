set(TARGET_NAME desktop_dupl)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../OpenVROverlay_imgui)
add_executable(${TARGET_NAME}
    main.cpp
    overlay.cpp
    duplicator.cpp
    compileshader.cpp
    quad.cpp
    quad.hlsl
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/headers
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    _UNICODE
    UNICODE
    )
target_link_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/openvr/lib/win64
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    openvr_api
    d3d11
    dxgi
    d3dcompiler
    )

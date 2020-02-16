set(TARGET_NAME vrcui)
add_executable(${TARGET_NAME}
    main.cpp
    )
target_include_directories(${TARGET_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/include
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/lib/openvr_api.lib
    )

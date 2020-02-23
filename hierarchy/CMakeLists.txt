set(TARGET_NAME hierarchy)
add_library(${TARGET_NAME}
    SceneMesh.cpp
    SceneCamera.cpp
    Scene.cpp
    GltfLoader.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} 
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
    ${EXTERNAL_DIR}/sukonbu/gltfformat/include
    ${EXTERNAL_DIR}/sukonbu/gltfformat/nlohmann-json/single_include
    )
target_link_libraries(${TARGET_NAME} PUBLIC
    screenstate
    )
target_compile_definitions(${TARGET_NAME} PUBLIC
    NOMINMAX
    )

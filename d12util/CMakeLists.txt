set(TARGET_NAME d12util)
add_library(${TARGET_NAME}
    CommandQueue.cpp
    RenderTarget.cpp
    SwapChain.cpp
    CommandList.cpp
    ResourceItem.cpp
    Uploader.cpp
    Mesh.cpp
    RootSignature.cpp
    Heap.cpp
    SceneMapper.cpp
    Shader.cpp
    Texture.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_link_libraries(${TARGET_NAME} PUBLIC
    hierarchy
    )

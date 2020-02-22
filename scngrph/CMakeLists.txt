set(TARGET_NAME scngrph)
add_library(${TARGET_NAME}
    Model.cpp
    Camera.cpp
    Scene.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_link_libraries(${TARGET_NAME} PUBLIC
    screenstate
    )
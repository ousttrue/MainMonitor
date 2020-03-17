cmake_minimum_required(VERSION 3.0.0)
project(MainMonitor)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)
subdirs(
    ${EXTERNAL_DIR}/gizmesh/falg
    ${EXTERNAL_DIR}/gizmesh/gizmesh
    ${EXTERNAL_DIR}/gizmesh/example/window_example
    ${EXTERNAL_DIR}/gizmesh/example/camera_example
    imgui 
    d12util 
    hierarchy 
    desktop_dupl 
    ExternalViewer 
    vrcui
    )

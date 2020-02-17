cmake_minimum_required(VERSION 3.0.0)
project(MainMonitor)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)
subdirs(imgui d12util desktop_dupl ExternalViewer vrcui)

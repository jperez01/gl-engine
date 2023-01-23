cmake_minimum_required(VERSION 3.8)

project("GL Engine")
set(CMAKE_CXX_STANDARD 17)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(third_party)
add_subdirectory(src)
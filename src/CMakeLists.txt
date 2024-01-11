add_library(gl_tools
    core/application.cpp

    engine/gl_base_engine.cpp
    engine/gl_physics.cpp

    ui/editor.cpp

    utils/material.cpp
    utils/gl_funcs.cpp
    utils/camera.cpp
    utils/gl_model.cpp
    utils/shader.cpp
    utils/gl_types.cpp
    utils/gl_compute.cpp 
    ui/ui.cpp)

add_executable(gl_engine
    main.cpp
    engine/gl_engine.cpp
    ${APP_ICON_RESOURCE_WINDOWS})

add_executable(compute_engine
    exes/computeDemo.cpp
    engine/gl_compute_eng.cpp)

add_executable(deferred_engine
    exes/deferredDemo.cpp
    engine/gl_deferred_eng.cpp)

add_executable(pbr_engine
    exes/pbrDemo.cpp
    engine/gl_pbr_engine.cpp)

add_executable(clustered_engine
    exes/clusteredDemo.cpp
    engine/gl_clustered_engine.cpp)

add_executable(voxel_cone_tracing
    exes/voxelDemo.cpp
    engine/gl_voxel_eng.cpp)

add_executable(indirect_rendering
    exes/indirectDemo.cpp
    engine/gl_indirect_eng.cpp)

add_executable(cloud_rendering
    exes/cloudDemo.cpp
    engine/cloud_eng.cpp)

target_include_directories(gl_tools PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

target_include_directories(gl_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

# Assimp from vcpkg or other package manager
find_package(assimp CONFIG REQUIRED)
# Assimp from source

target_link_libraries(gl_tools PUBLIC glad glm stb_image imgui imGuizmo sdl2 assimp::assimp)

target_link_libraries(gl_engine gl_tools)
target_link_libraries(compute_engine gl_tools)
target_link_libraries(deferred_engine gl_tools)
target_link_libraries(pbr_engine gl_tools)
target_link_libraries(clustered_engine gl_tools)
target_link_libraries(voxel_cone_tracing gl_tools)
target_link_libraries(indirect_rendering gl_tools)
target_link_libraries(cloud_rendering gl_tools)
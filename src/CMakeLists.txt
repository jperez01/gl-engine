add_library(gl_tools
    engine/gl_base_engine.cpp
    engine/gl_physics.cpp

    utils/gl_funcs.cpp
    utils/camera.cpp
    utils/gl_model.cpp
    utils/shader.cpp
    utils/gl_types.cpp
    utils/gl_compute.cpp)

add_executable(gl_engine
    main.cpp
    engine/gl_engine.cpp)

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

target_include_directories(gl_tools PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

target_include_directories(gl_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

set (GLFW_LIB "${PROJECT_SOURCE_DIR}/libs/GLFW/glfw3.lib")
set (ASSIMP_LIB "${PROJECT_SOURCE_DIR}/libs/assimp/assimp.lib")
target_link_libraries(gl_tools glad glm stb_image imgui imGuizmo sdl2 ${GLFW_LIB} ${ASSIMP_LIB})

target_link_libraries(gl_engine gl_tools)
target_link_libraries(compute_engine gl_tools)
target_link_libraries(deferred_engine gl_tools)
target_link_libraries(pbr_engine gl_tools)
target_link_libraries(clustered_engine gl_tools)
target_link_libraries(voxel_cone_tracing gl_tools)
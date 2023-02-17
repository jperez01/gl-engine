add_library(gl_tools
    engine/gl_engine.h
    engine/gl_engine.cpp
    engine/gl_base_engine.h
    engine/gl_base_engine.cpp
    engine/gl_compute_eng.h
    engine/gl_compute_eng.cpp
    engine/gl_deferred_eng.h
    engine/gl_deferred_eng.cpp
    engine/gl_funcs.h
    engine/gl_funcs.cpp

    utils/camera.h
    utils/camera.cpp
    utils/gl_model.h
    utils/gl_model.cpp
    utils/shader.h
    utils/shader.cpp
    utils/gl_types.h
    utils/gl_types.cpp
    utils/gl_compute.h
    utils/gl_compute.cpp)

add_executable(gl_engine
    main.cpp)

add_executable(compute_engine
    exes/computeDemo.cpp)

add_executable(deferred_engine
    exes/deferredDemo.cpp)

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
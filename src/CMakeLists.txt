add_executable(gl_engine
    main.cpp
    engine/gl_engine.h
    engine/gl_engine.cpp
    utils/camera.h
    utils/camera.cpp
    utils/gl_model.h
    utils/gl_model.cpp
    utils/shader.h
    utils/shader.cpp
    utils/gl_types.h
)

target_include_directories(gl_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

set (GLFW_LIB "${PROJECT_SOURCE_DIR}/libs/GLFW/glfw3.lib")
set (ASSIMP_LIB "${PROJECT_SOURCE_DIR}/libs/assimp/assimp.lib")
target_link_libraries(gl_engine glad glm stb_image imgui sdl2 ${GLFW_LIB} ${ASSIMP_LIB})
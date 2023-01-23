add_executable(gl_engine
    main.cpp
)

target_include_directories(gl_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include)

set (GLFW_LIB "${PROJECT_SOURCE_DIR}/libs/GLFW/glfw3.lib")
target_link_libraries(gl_engine glad ${GLFW_LIB})
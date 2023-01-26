add_library(glad STATIC)
target_sources(glad PRIVATE
    glad/glad.c)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

add_library(stb_image STATIC)
target_sources(stb_image PRIVATE
    stb_image/image.cpp)
target_include_directories(stb_image INTERFACE stb_image)

target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_library(sdl2 INTERFACE)
set(sdl2_DIR "SDL_PATH" CACHE PATH "Path to SDL2")

if (WIN32)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
elseif (UNIX)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
endif()

target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
    imgui/imgui.h
    imgui/imgui.cpp

    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_stdlib.cpp

    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_sdl.cpp)

target_link_libraries(imgui PUBLIC sdl2)
# GL Engine

OpenGL renderer written in C++. This is a personal project for me to learn about Graphics Programming and modern features of the OpenGL API. It has basic demos of rendering topics such as Voxel Cone Tracing and Clustered Lighting.

## Getting Started

### Prerequisites

This project uses CMake as the build system. Also certain external libraries are necessary:
* [Assimp](https://github.com/assimp/assimp) - Asset Importer (provided as DLL/Lib or can be installed through package manager like vcpkg)
* [SDL2](https://wiki.libsdl.org/SDL2/Installation) - Window Management (must be externally installed and then linked to CMake variable SDL_DIR)

### Build

After setting up the libraries above, you can build through CMake to get the executables. If you open the executable and receive an error that a DLL was not provided, you can look in the `libs` folder for the necessary file.

# Features
* Deferred Rendering
* PBR Shader
* Voxel Cone Tracing
* Clustered Forward Lighting
* Simple Animations
* Post Processing
    * FXAA
    * TAA


## Built With

* [GLM](http://www.dropwizard.io/1.0.2/docs/) - Math Library
* [GLAD](https://github.com/Dav1dde/glad) - OpenGL function linker
* [ImGui](https://github.com/ocornut/imgui) - editor UI
* [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) - Guizmo extension to ImGui
* [Assimp](https://github.com/assimp/assimp) - Asset Importer
* [SDL2](https://github.com/libsdl-org/SDL) - Window Management


## Acknowledgments

* [LearnOpenGL](https://learnopengl.com/)
* [Clustered Lighting Primer](http://www.aortiz.me/2018/12/21/CG.html)
* [Real Time Rendering](https://www.realtimerendering.com/)

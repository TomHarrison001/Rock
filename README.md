# Rock
<strong>A game engine built with Vulkan</strong>

- To run the engine, compute app or example game:
  - Set Renderer as startup project
  - Change active application in Renderer entry point (main.cpp)
- To run the testing:
  - Set Testing as startup project

> Run `setup.bat` to compile shaders.

> Run `setup.bat` to run Doxygen.

## Required

|Name|Version|
-----|-----
|[Vulkan SDK](https://vulkan.lunarg.com/sdk/home)|1.4.309.0|

If using a different version of Vulkan SDK, change the Additional Include Directories (Configuration Properties > C/C++) and Additional Library Directories (Configuration Properties > Linker) to include that version for the Renderer, Physics and Testing projects.

## Dependencies

|Name|Notes|
-----|-----
|GLFW|lib included|
|glm|headers included|
|stb_image|header included|
|tinyobjloader|header included|
|imgui|headers included|
|gtests|lib included|
|EnTT|header included|

## Built with

|Name|Version|
-----|-----
|C++|ISO C++17|
|Visual Studio|2022 (v143)|

:: turn echo off
@echo off

:: compile shaders to SPIR-V
echo Compiling shaders...
@echo on

C:/VulkanSDK/1.4.309.0/Bin/glslc.exe ./Renderer/res/shaders/engineApp/main.vert -o ./Renderer/res/shaders/engineApp/vert.spv
C:/VulkanSDK/1.4.309.0/Bin/glslc.exe ./Renderer/res/shaders/engineApp/main.frag -o ./Renderer/res/shaders/engineApp/frag.spv
C:/VulkanSDK/1.4.309.0/Bin/glslc.exe ./Renderer/res/shaders/computeApp/main.vert -o ./Renderer/res/shaders/computeApp/vert.spv
C:/VulkanSDK/1.4.309.0/Bin/glslc.exe ./Renderer/res/shaders/computeApp/main.frag -o ./Renderer/res/shaders/computeApp/frag.spv
C:/VulkanSDK/1.4.309.0/Bin/glslc.exe ./Renderer/res/shaders/computeApp/main.comp -o ./Renderer/res/shaders/computeApp/comp.spv

:: turn echo off
@echo off
echo:

:: run doxygen for documentation
echo Running Doxygen...
echo:

call "./Documentation/bin/doxygen.exe" "./Documentation/Doxyfile"

echo:
pause

@echo off
if not exist build (mkdir build)
cd build
cmake ..
cd ..
echo You can now open Application.sln in the build directory

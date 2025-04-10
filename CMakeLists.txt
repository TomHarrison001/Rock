cmake_minimum_required(VERSION 3.19)

project(Rock CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ROCK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROCK_DEPS_CACHE_FILE "${ROCK_ROOT_DIR}/deps/.bootstrap.json")

find_package(Python3 COMPONENTS Interpreter)

execute_process(COMMAND ${Python3_EXECUTABLE} deploy_deps.py WORKING_DIRECTORY ${ROCK_ROOT_DIR} COMMAND_ERROR_IS_FATAL ANY)
add_custom_command(COMMAND ${Python3_EXECUTABLE} deploy_deps.py OUTPUT ${ROCK_DEPS_CACHE_FILE} DEPENDS "${ROCK_ROOT_DIR}/deps/bootstrap.json" WORKING_DIRECTORY ${ROCK_ROOT_DIR})
add_custom_target(Dependencies DEPENDS ${ROCK_DEPS_CACHE_FILE})
set_property(TARGET Dependencies PROPERTY FOLDER "third-party")

if(NOT EXISTS ${ROCK_DEPS_CACHE_FILE})
    message(FATAL_ERROR "Cannot bootstrap dependencies.")
endif()

# cmake-format: off
set(LVK_WITH_SAMPLES OFF CACHE BOOL "")
set(LVK_WITH_TRACY   ON  CACHE BOOL "")
add_subdirectory(deps/src/lightweightvk)
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/bc7enc)
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/ldrutils)
set(KTX_FEATURE_DOC            OFF CACHE BOOL "")
set(KTX_FEATURE_GL_UPLOAD      OFF CACHE BOOL "")
set(KTX_FEATURE_JNI            OFF CACHE BOOL "")
set(KTX_FEATURE_KTX1            ON CACHE BOOL "")
set(KTX_FEATURE_KTX2            ON CACHE BOOL "")
set(KTX_FEATURE_LOADTEST_APPS  OFF CACHE BOOL "")
set(KTX_FEATURE_STATIC_LIBRARY  ON CACHE BOOL "")
set(KTX_FEATURE_TESTS          OFF CACHE BOOL "")
set(KTX_FEATURE_TOOLS          OFF CACHE BOOL "")
set(KTX_FEATURE_VK_UPLOAD      OFF CACHE BOOL "")
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/ktx-software)
lvk_set_folder(ktx "third-party/ktx-software")
lvk_set_folder(ktx_read "third-party/ktx-software")
lvk_set_folder(ktx_version "third-party/ktx-software")
lvk_set_folder(obj_basisu_cbind "third-party/ktx-software")
lvk_set_folder(objUtil "third-party/ktx-software")
lvk_set_folder(fmt "third-party")
lvk_set_folder(astcenc-avx2-static "third-party/ktx-software")
lvk_set_folder(makedfd2vk "third-party/ktx-software")
lvk_set_folder(makevk2dfd "third-party/ktx-software")
lvk_set_folder(mkvk "third-party/ktx-software")
lvk_set_folder(mkvkformatfiles "third-party/ktx-software")
# cmake-format: on

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")

add_subdirectory(deps/src/assimp)
if(APPLE)
    target_compile_options(assimp PRIVATE -Wno-deprecated-declarations)
endif()

set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "")
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "")
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(deps/src/meshoptimizer)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(.)
include_directories(deps/src)
include_directories(deps/src/taskflow)
include_directories(deps/src/glm)
#include_directories(deps/src/rapidjson/include)
#include_directories(deps/src/bullet/src)
#include_directories(deps/src/ImGuizmo/src)

#add_subdirectory(deps/cmake/bullet)
#add_subdirectory(deps/cmake/ImGuizmo)

set_property(TARGET assimp          PROPERTY FOLDER "third-party")
set_property(TARGET bc7enc          PROPERTY FOLDER "third-party")
set_property(TARGET LUtils          PROPERTY FOLDER "third-party")
set_property(TARGET meshoptimizer   PROPERTY FOLDER "third-party")
if(WIN32)
    set_property(TARGET zlibstatic    PROPERTY FOLDER "third-party")
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "third-party")
endif()
set_property(TARGET uninstall       PROPERTY FOLDER "third-party")

# shared utils code
add_subdirectory(shared)
#add_subdirectory(data)

add_dependencies(SharedUtils Dependencies)

# disable warnings in third-party code
if(MSVC)
    target_compile_options(assimp PRIVATE /wd4267)
    target_compile_options(SPIRV  PRIVATE /wd4267)
endif()

add_subdirectory(application)
set_property(DIRECTORY ${ROCK_ROOT_DIR} PROPERTY VS_STARTUP_PROJECT application)

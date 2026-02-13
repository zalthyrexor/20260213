cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "")

project(SF_Car_Sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib CONFIG REQUIRED)
find_package(entt CONFIG REQUIRED)

add_executable(SF_Car_Sim 
    src/main.cpp 
     
    "src/ChunkMeshBuilder.h" 
    "src/stb_perlin.h" 
    "src/ChunkManager.h"
)

add_custom_command(TARGET SF_Car_Sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:SF_Car_Sim>/resources"
)

target_link_libraries(SF_Car_Sim PRIVATE 
    raylib 
    EnTT::EnTT
)
project(Pano2Skybox)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

#OS X port
#include_directories(/opt/local/include)
#include_directories(/opt/local/lib/glib-2.0/include)
#link_directories(/opt/local/lib/)

#OS X brew
include_directories(/usr/local/opt/jpeg-turbo/include/)
#include_directories(/usr/local/opt/glib/lib/glib-2.0/include)
link_directories(/usr/local/lib)

#Win MSYS2
include_directories(c:/opt/msys64/mingw64/include/)
include_directories(c:/opt/msys64/mingw64/lib/glib-2.0/include)

#Linux
include_directories(/usr/include/)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
link_directories(/usr/lib/)


set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")


if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -Wall")
else( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif( CMAKE_BUILD_TYPE STREQUAL "Debug" )

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} jpeg)




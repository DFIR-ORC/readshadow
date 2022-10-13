cmake_minimum_required(VERSION 3.22)

project(readshadow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(readshadow readshadow.cpp)
set_property(TARGET readshadow PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(findshadow findshadow.cpp)
set_property(TARGET findshadow PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

cmake_minimum_required(VERSION 3.10)

project(tcp-compression-service VERSION 0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(project-config.h.in project-config.h)

add_executable(tcp-compression-service src/main.cpp)

target_include_directories(tcp-compression-service PRIVATE
                           "${PROJECT_SOURCE_DIR}/include"
                           )
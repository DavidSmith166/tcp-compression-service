cmake_minimum_required(VERSION 3.10)

project(tcp-compression-service VERSION 0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_COMPILE_WARNING_MODE HIGH)
set(CMAKE_COMPILE_WARNING_AS_ERROR TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter=.;
    "-checks=cppcoreguidelines-*,\
    google-*,\
    misc-*,\
    clang-*,\
    -cppcoreguidelines-pro-bounds-pointer-arithmetic,\
    -cppcoreguidelines-pro-bounds-constant-array-index,\
    -cppcoreguidelines-pro-bounds-array-to-pointer-decay,\
    -cppcoreguidelines-pro-type-member-init,\
    -cppcoreguidelines-special-member-functions,\
    -cppcoreguidelines-pro-type-reinterpret-cast,\
    -cppcoreguidelines-pro-type-vararg"
   )

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

configure_file(project-config.h.in project-config.h)

add_executable(tcp-compression-service 
               src/main.cpp
               src/service.cpp
               src/listener.cpp
               src/worker.cpp
               src/compression.cpp
              )

target_include_directories(tcp-compression-service PRIVATE
                           "${PROJECT_SOURCE_DIR}/include"
                          )

target_link_libraries(tcp-compression-service Threads::Threads)

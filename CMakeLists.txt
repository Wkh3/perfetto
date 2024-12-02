cmake_minimum_required(VERSION 3.25)

project(perfetto)

find_package(Threads)

set(CMAKE_CXX_STANDARD 17)

add_library(perfetto STATIC ./sdk/perfetto.cc)

target_include_directories(perfetto PUBLIC ./sdk)

target_link_libraries(perfetto PUBLIC ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
    target_compile_options(perfetto PRIVATE "/bigobj")

    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)

    target_link_libraries(perfetto PUBLIC ws2_32)
endif()

if(MSVC)
    target_compile_options(perfetto PUBLIC "/permissive-")
endif()
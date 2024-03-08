cmake_minimum_required(VERSION 3.20)
project(sabogae
        VERSION 1.0
        DESCRIPTION "TBD"
        LANGUAGES CXX)

include(cmake/get_cpm.cmake)
include(cmake/setup_ccache.cmake)

add_library(sabogae STATIC
        include/sabogae/ds/prio_list.h
        include/sabogae/util/averagers.h
        include/sabogae/util/time.h
        include/sabogae/sabogae.h

        src/sabogae/ds/prio_list.cpp
        src/sabogae/util/averagers.cpp
        src/sabogae/util/time.cpp
        src/sabogae/sabogae.cpp)

target_include_directories(sabogae PUBLIC include)

target_compile_features(sabogae PUBLIC cxx_std_23)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(sabogae PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_compile_options(sabogae PUBLIC /utf-8 /W4)
endif ()

include(cmake/get_thirdparty.cmake)
target_link_libraries(sabogae PUBLIC sabogae_thirdparty)

if (WIN32)
    target_link_libraries(sabogae PUBLIC ntdll imm32.lib Dwmapi.lib opengl32.lib)
endif (WIN32)

add_subdirectory(example)



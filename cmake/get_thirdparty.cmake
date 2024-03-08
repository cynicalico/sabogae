set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 10.1.1
)

add_library(sabogae_thirdparty INTERFACE)

target_link_libraries(sabogae_thirdparty INTERFACE
        fmt::fmt)

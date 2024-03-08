set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(../cmake/auto_targets.cmake)
auto_targets(BASE_DIR ${CMAKE_CURRENT_LIST_DIR} TYPE_STR "example" LIBS sabogae)

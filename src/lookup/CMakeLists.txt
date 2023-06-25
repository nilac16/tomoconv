if (WIN32)
    add_library(lookup lookup-win32.cpp)

    target_link_libraries(lookup PRIVATE ws2_32.lib)

    set_property(TARGET lookup PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else ()
    add_library(lookup lookup-unix.cpp)
endif ()

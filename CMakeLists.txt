cmake_minimum_required(VERSION 3.16)

project(overlay LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)


pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-video-1.0
)


pkg_check_modules(GST_APP REQUIRED
    gstreamer-app-1.0
)


pkg_check_modules(CAIRO REQUIRED
    cairo
)


add_executable(
    overlay
    src/main.cpp
)


target_include_directories(overlay PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_directories(overlay PRIVATE
    ${GST_LIBRARY_DIRS}
    ${GST_APP_LIBRARY_DIRS}
    ${CAIRO_LIBRARY_DIRS}
)


target_compile_options(overlay PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${CAIRO_CFLAGS_OTHER}
)


target_link_libraries(overlay PRIVATE
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${CAIRO_LIBRARIES}
)

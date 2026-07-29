cmake_minimum_required(VERSION 3.16)

project(overlay LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# -------------------------------------------------
# Dependencies
# -------------------------------------------------

find_package(PkgConfig REQUIRED)


pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-video-1.0
    gstreamer-app-1.0
)


pkg_check_modules(CAIRO REQUIRED
    cairo
)



# -------------------------------------------------
# Main application
# -------------------------------------------------

add_executable(
    overlay
    src/main.cpp
)


target_include_directories(overlay PRIVATE

    ${GST_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}

    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_directories(overlay PRIVATE

    ${GST_LIBRARY_DIRS}
    ${CAIRO_LIBRARY_DIRS}
)


target_compile_options(overlay PRIVATE

    ${GST_CFLAGS_OTHER}
    ${CAIRO_CFLAGS_OTHER}
)


target_link_libraries(overlay PRIVATE

    ${GST_LIBRARIES}
    ${CAIRO_LIBRARIES}
)



# -------------------------------------------------
# Unit Tests
# -------------------------------------------------

enable_testing()


find_package(GTest REQUIRED)



add_executable(
    overlay_test

    tests/OverlayTest.cpp
)



target_include_directories(overlay_test PRIVATE

    ${GST_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}

    ${CMAKE_CURRENT_SOURCE_DIR}/src
)



target_link_directories(overlay_test PRIVATE

    ${GST_LIBRARY_DIRS}
    ${CAIRO_LIBRARY_DIRS}
)



target_compile_options(overlay_test PRIVATE

    ${GST_CFLAGS_OTHER}
    ${CAIRO_CFLAGS_OTHER}
)



target_link_libraries(overlay_test PRIVATE

    GTest::gtest

    ${GST_LIBRARIES}
    ${CAIRO_LIBRARIES}
)



include(GoogleTest)


gtest_discover_tests(
    overlay_test
)

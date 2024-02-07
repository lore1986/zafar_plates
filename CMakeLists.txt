cmake_minimum_required(VERSION 3.10)
project(plates-zafar C CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(ggtk REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0)
pkg_search_module(gstreamer-base REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_search_module(gstreamer-controller REQUIRED IMPORTED_TARGET gstreamer-controller-1.0)

add_executable(plates-zafar plates-zafar.cpp)

target_include_directories(plates-zafar PRIVATE ${GTK_INCLUDE_DIRS})
target_link_directories(plates-zafar PRIVATE ${GTK_LIBRARY_DIRS})

target_link_libraries(plates-zafar PRIVATE 
        ${GTK_LIBRARIES}
        PkgConfig::glib
        PkgConfig::gstreamer
        PkgConfig::gstreamer-sdp
        PkgConfig::gstreamer-app
        PkgConfig::gstreamer-video
        PkgConfig::gstreamer-base
        PkgConfig::gstreamer-controller
)


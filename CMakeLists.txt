cmake_minimum_required(VERSION 3.16)
project(MediaViewer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets OpenGL OpenGLWidgets Concurrent)

# Find libzip - check local deps first, then system
set(LIBZIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/libzip/include")
set(LIBZIP_LIBRARY "${CMAKE_SOURCE_DIR}/deps/libzip/lib/libzip.dll.a")

if(NOT EXISTS "${LIBZIP_INCLUDE_DIR}/zip.h" OR NOT EXISTS "${LIBZIP_LIBRARY}")
    # Fall back to pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBZIP REQUIRED libzip)
else()
    message(STATUS "Using local libzip: ${LIBZIP_INCLUDE_DIR}")
    set(LIBZIP_INCLUDE_DIRS ${LIBZIP_INCLUDE_DIR})
    set(LIBZIP_LIBRARIES ${LIBZIP_LIBRARY})
endif()

add_executable(MediaViewer
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mediaitem.cpp
    mediaitem.h
    imageviewer.cpp
    imageviewer.h
    zipreader.cpp
    zipreader.h
    zipmediaitem.cpp
    zipmediaitem.h
    zipimageviewer.cpp
    zipimageviewer.h
)

target_include_directories(MediaViewer PRIVATE
    ${LIBZIP_INCLUDE_DIRS}
)

target_link_libraries(MediaViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Concurrent
    ${LIBZIP_LIBRARIES}
)

# Install target
install(TARGETS MediaViewer
    RUNTIME DESTINATION bin
)

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

add_executable(MediaViewer
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mediaitem.cpp
    mediaitem.h
    imageviewer.cpp
    imageviewer.h
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
)

# Install target
install(TARGETS MediaViewer
    RUNTIME DESTINATION bin
)

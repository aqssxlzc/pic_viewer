cmake_minimum_required(VERSION 3.16)
project(MediaViewer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Optimization flags (only for Release builds)
# Note: -march=native removed for CI compatibility
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Disable Vulkan requirement (not needed for this project)
set(Vulkan_FOUND OFF CACHE BOOL "" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets OpenGL OpenGLWidgets Concurrent)

# Find libzip - check local deps first, then system/vcpkg
set(LIBZIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/libzip/include")
set(LIBZIP_LIBRARY "${CMAKE_SOURCE_DIR}/deps/libzip/lib/libzip.dll.a")

if(EXISTS "${LIBZIP_INCLUDE_DIR}/zip.h" AND EXISTS "${LIBZIP_LIBRARY}")
    message(STATUS "Using local libzip: ${LIBZIP_INCLUDE_DIR}")
    set(LIBZIP_INCLUDE_DIRS ${LIBZIP_INCLUDE_DIR})
    set(LIBZIP_LIBRARIES ${LIBZIP_LIBRARY})
elseif(WIN32)
    # Windows: use vcpkg cmake config
    find_package(libzip CONFIG REQUIRED)
    set(LIBZIP_INCLUDE_DIRS "")
    set(LIBZIP_LIBRARIES libzip::zip)
else()
    # Linux/macOS: use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBZIP REQUIRED libzip)
    # On macOS, also set library directory for linker
    if(APPLE)
        link_directories(${LIBZIP_LIBRARY_DIRS})
    endif()
endif()

add_executable(MediaViewer MACOSX_BUNDLE
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
    resources.qrc
)

# Windows icon embedding
if(WIN32)
    # Use resource file for Windows icon
    target_sources(MediaViewer PRIVATE resources.rc)
    set_target_properties(MediaViewer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# macOS bundle metadata — required for a valid .app that macOS will launch
if(APPLE)
    set_target_properties(MediaViewer PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER  "com.mediaviewer.app"
        MACOSX_BUNDLE_BUNDLE_NAME     "MediaViewer"
        MACOSX_BUNDLE_BUNDLE_VERSION  ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_COPYRIGHT       "Copyright 2024"
    )
endif()

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

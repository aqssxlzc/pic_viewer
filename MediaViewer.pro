QT       += core gui widgets multimedia multimediawidgets concurrent opengl openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Optimization flags (-march=native removed; not safe for distribution)
QMAKE_CXXFLAGS += -O3
CONFIG += optimize_full

TARGET = MediaViewer
TEMPLATE = app

# Link libzip via pkg-config (Linux / macOS)
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libzip

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mediaitem.cpp \
    imageviewer.cpp \
    zipreader.cpp \
    zipmediaitem.cpp \
    zipimageviewer.cpp

HEADERS += \
    mainwindow.h \
    mediaitem.h \
    imageviewer.h \
    zipreader.h \
    zipmediaitem.h \
    zipimageviewer.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

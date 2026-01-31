QT       += core gui widgets multimedia multimediawidgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Optimization flags
QMAKE_CXXFLAGS += -O3 -march=native
CONFIG += optimize_full

# Enable OpenGL for hardware acceleration
QT += opengl

TARGET = MediaViewer
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mediaitem.cpp \
    imageviewer.cpp

HEADERS += \
    mainwindow.h \
    mediaitem.h \
    imageviewer.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

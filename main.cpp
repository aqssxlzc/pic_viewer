#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QThreadPool>
#include <QThread>

int main(int argc, char *argv[])
{
    // Enable hardware acceleration
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1); // VSync
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("Media Viewer");
    app.setApplicationVersion("1.0");
    
    // 设置线程池大小为 CPU 核心数的 2 倍，加速图片预加载
    int threadCount = QThread::idealThreadCount() * 2;
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(8, threadCount));
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

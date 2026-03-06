#include "imageviewer.h"
#include <QVBoxLayout>
#include <QImageReader>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFileInfo>
#include <QtConcurrent>
#include <QUrl>

ImageViewer::ImageViewer(const QString &filePath, const QStringList &allPaths, QWidget *parent)
    : QDialog(parent)
    , mediaPaths(allPaths)
    , currentIndex(allPaths.indexOf(filePath))
    , isVideo(false)
    , currentLabelIndex(0)
    , videoPlayTimer(nullptr)
    , audioOutput(nullptr)
{
    setupUI();

    // 先设置全屏，确保 size() 返回正确的全屏尺寸
    setWindowState(Qt::WindowFullScreen);
    setModal(true);

    // 全屏后再加载媒体和预加载
    loadMedia();
}

ImageViewer::~ImageViewer()
{
    // 取消所有异步加载任务
    for (auto watcher : preloadWatchers) {
        if (watcher) {
            watcher->cancel();
            disconnect(watcher, nullptr, nullptr, nullptr);
            if (!watcher->isFinished()) {
                watcher->waitForFinished();
            }
            delete watcher;
        }
    }
    preloadWatchers.clear();
    pixmapCache.clear();

    if (videoPlayTimer) {
        videoPlayTimer->stop();
        delete videoPlayTimer;
    }
}

void ImageViewer::setupUI()
{
    setStyleSheet("background-color: rgba(0, 0, 0, 240);");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 使用 QStackedWidget 实现双缓冲，避免黑屏
    imageStack = new QStackedWidget(this);
    mainLayout->addWidget(imageStack);

    // 创建多个 QLabel 用于双缓冲
    for (int i = 0; i < 3; ++i) {
        QLabel *label = new QLabel(imageStack);
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(false);
        label->setStyleSheet("background-color: transparent;");
        imageLabels.append(label);
        imageStack->addWidget(label);
    }
    imageStack->setCurrentIndex(0);

    // Video widget
    videoWidget = new QVideoWidget(this);
    videoWidget->setStyleSheet("background-color: black;");
    videoWidget->hide();
    mainLayout->addWidget(videoWidget);

    // Media player - 只创建一个实例
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setVideoOutput(videoWidget);
    mediaPlayer->setAudioOutput(audioOutput);

    // 视频延迟播放定时器
    videoPlayTimer = new QTimer(this);
    videoPlayTimer->setSingleShot(true);
    connect(videoPlayTimer, &QTimer::timeout, this, &ImageViewer::playPendingVideo);

    // Close button
    closeButton = new QPushButton("✕", this);
    closeButton->setFixedSize(50, 50);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 25px;"
        "    font-size: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    closeButton->move(width() - 70, 20);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    // Previous button
    prevButton = new QPushButton("‹", this);
    prevButton->setFixedSize(60, 60);
    prevButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 30px;"
        "    font-size: 32px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    prevButton->move(20, height() / 2 - 30);
    connect(prevButton, &QPushButton::clicked, this, &ImageViewer::showPrevious);

    // Next button
    nextButton = new QPushButton("›", this);
    nextButton->setFixedSize(60, 60);
    nextButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 30px;"
        "    font-size: 32px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    nextButton->move(width() - 80, height() / 2 - 30);
    connect(nextButton, &QPushButton::clicked, this, &ImageViewer::showNext);

    // Play/Pause button for videos
    playPauseButton = new QPushButton("⏸", this);
    playPauseButton->setFixedSize(60, 60);
    playPauseButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 30px;"
        "    font-size: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    playPauseButton->hide();
    playPauseButton->move(width() / 2 - 30, height() - 100);
    connect(playPauseButton, &QPushButton::clicked, [this]() {
        if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            mediaPlayer->pause();
            playPauseButton->setText("▶");
        } else {
            mediaPlayer->play();
            playPauseButton->setText("⏸");
        }
    });

    // Add fade-in animation
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);

    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void ImageViewer::loadMedia()
{
    if (currentIndex < 0 || currentIndex >= mediaPaths.size()) {
        return;
    }

    QString filePath = mediaPaths[currentIndex];
    bool wasVideo = isVideo;
    isVideo = isVideoFile(filePath);

    // 停止任何待播放的视频
    videoPlayTimer->stop();
    pendingVideoPath.clear();

    // 先停止视频播放并清理资源
    if (wasVideo || isVideo) {
        mediaPlayer->stop();
        // 清除当前媒体源，释放 GStreamer 资源
        mediaPlayer->setSource(QUrl());
    }

    if (isVideo) {
        displayVideo(currentIndex);
    } else {
        displayImage(currentIndex);
        // 预加载相邻图片
        preloadAdjacent();
    }

    QString fileType = isVideo ? "视频" : "图片";
    setWindowTitle(QString("%1查看器 - %2/%3").arg(fileType).arg(currentIndex + 1).arg(mediaPaths.size()));
}

void ImageViewer::displayImage(int index)
{
    // Show image stack, hide video widget
    videoWidget->hide();
    playPauseButton->hide();
    imageStack->show();

    QString filePath = mediaPaths[index];

    // 检查缓存中是否有预加载的图片（缓存的是原始尺寸，需要缩放显示）
    {
        QMutexLocker locker(&cacheMutex);
        if (pixmapCache.contains(index)) {
            currentPixmap = pixmapCache[index];
            updateImage();
            return;
        }
    }

    // 同步加载当前图片

    // 同步加载当前图片
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (!image.isNull()) {
        currentPixmap = QPixmap::fromImage(image);
        qDebug() << "displayImage: loaded, index=" << index 
                 << "pixmap size=" << currentPixmap.size() 
                 << "window size=" << size()
                 << "screen size=" << (QApplication::primaryScreen() ? QApplication::primaryScreen()->geometry().size() : QSize());
        updateImage();
    }
}

void ImageViewer::displayVideo(int index)
{
    QString filePath = mediaPaths[index];

    // Show video widget, hide image stack
    imageStack->hide();
    videoWidget->show();
    playPauseButton->show();
    playPauseButton->setText("⏸");

    // 使用延迟加载避免 GStreamer 资源竞争
    pendingVideoPath = filePath;
    videoPlayTimer->start(100);  // 100ms 延迟
}

void ImageViewer::playPendingVideo()
{
    if (pendingVideoPath.isEmpty()) {
        return;
    }

    QString filePath = pendingVideoPath;
    pendingVideoPath.clear();

    // 设置新的媒体源
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));

    // Loop video
    disconnect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, nullptr, nullptr);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
        }
    }, Qt::UniqueConnection);

    mediaPlayer->play();
}

bool ImageViewer::isVideoFile(const QString &filePath) const
{
    QStringList videoExts;
    videoExts << "mp4" << "avi" << "mkv" << "mov" << "wmv" << "flv" << "webm";

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    return videoExts.contains(ext);
}

void ImageViewer::updateVideo()
{
    // Video size is handled automatically by QVideoWidget
}

void ImageViewer::updateImage()
{
    if (currentPixmap.isNull()) {
        return;
    }

    // 使用屏幕尺寸，而不是窗口尺寸（窗口可能还没有完全显示）
    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen ? screen->geometry().size() : size();

    // Scale image to fit screen while maintaining aspect ratio
    QPixmap scaledPixmap = currentPixmap.scaled(
        screenSize * 0.9,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    QLabel *label = getAvailableLabel();
    label->setPixmap(scaledPixmap);
    switchToLabel(label);
}

QLabel* ImageViewer::getAvailableLabel()
{
    // 获取下一个可用的标签（不是当前显示的）
    int nextIndex = (currentLabelIndex + 1) % imageLabels.size();
    return imageLabels[nextIndex];
}

void ImageViewer::switchToLabel(QLabel *label)
{
    int index = imageLabels.indexOf(label);
    if (index >= 0) {
        imageStack->setCurrentIndex(index);
        currentLabelIndex = index;
    }
}

void ImageViewer::showPrevious()
{
    if (mediaPaths.isEmpty()) {
        return;
    }

    currentIndex--;
    if (currentIndex < 0) {
        currentIndex = mediaPaths.size() - 1;
    }

    loadMedia();
}

void ImageViewer::showNext()
{
    if (mediaPaths.isEmpty()) {
        return;
    }

    currentIndex++;
    if (currentIndex >= mediaPaths.size()) {
        currentIndex = 0;
    }

    loadMedia();
}

void ImageViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
        case Qt::Key_Q:
            close();
            break;
        case Qt::Key_Left:
        case Qt::Key_A:
        case Qt::Key_Up:
        case Qt::Key_W:
            showPrevious();
            break;
        case Qt::Key_Right:
        case Qt::Key_D:
        case Qt::Key_Down:
        case Qt::Key_S:
            showNext();
            break;
        case Qt::Key_Home:
            currentIndex = 0;
            loadMedia();
            break;
        case Qt::Key_End:
            currentIndex = mediaPaths.size() - 1;
            loadMedia();
            break;
        case Qt::Key_PageUp:
            currentIndex = qMax(0, currentIndex - 10);
            loadMedia();
            break;
        case Qt::Key_PageDown:
            currentIndex = qMin(mediaPaths.size() - 1, currentIndex + 10);
            loadMedia();
            break;
        case Qt::Key_Space:
            if (isVideo) {
                if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
                    mediaPlayer->pause();
                    playPauseButton->setText("▶");
                } else {
                    mediaPlayer->play();
                    playPauseButton->setText("⏸");
                }
            }
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    // Reposition buttons
    closeButton->move(width() - 70, 20);
    prevButton->move(20, height() / 2 - 30);
    nextButton->move(width() - 80, height() / 2 - 30);
    playPauseButton->move(width() / 2 - 30, height() - 100);

    // Update media size
    if (isVideo) {
        updateVideo();
    } else if (!currentPixmap.isNull()) {
        QScreen *screen = QApplication::primaryScreen();
        QSize screenSize = screen ? screen->geometry().size() : size();
        QPixmap scaledPixmap = currentPixmap.scaled(
            screenSize * 0.9,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        QLabel *currentLabel = imageLabels[currentLabelIndex];
        currentLabel->setPixmap(scaledPixmap);
    }

    // 清除缓存，窗口大小变了需要重新缩放
    QMutexLocker locker(&cacheMutex);
    pixmapCache.clear();
}

// ==================== 预加载相关方法 ====================

void ImageViewer::preloadAdjacent()
{

    // 取消之前未完成的预加载
    for (auto it = preloadWatchers.begin(); it != preloadWatchers.end(); ) {
        if (it.value() && !it.value()->isFinished()) {
            it.value()->cancel();
            disconnect(it.value(), nullptr, nullptr, nullptr);
            it.value()->waitForFinished();
            delete it.value();
            it = preloadWatchers.erase(it);
        } else {
            ++it;
        }
    }

    // 预加载前后的图片文件
    for (int offset = -PRELOAD_RANGE; offset <= PRELOAD_RANGE; offset++) {
        if (offset == 0) continue;

        int preloadIndex = currentIndex + offset;
        if (preloadIndex < 0 || preloadIndex >= mediaPaths.size()) continue;

        QString filePath = mediaPaths[preloadIndex];
        if (!isVideoFile(filePath)) {
            QMutexLocker locker(&cacheMutex);
            if (pixmapCache.contains(preloadIndex)) {
                continue;
            }
            startPreload(preloadIndex);
        }
    }
}

void ImageViewer::startPreload(int index)
{
    if (index < 0 || index >= mediaPaths.size()) return;

    QString filePath = mediaPaths[index];

    QFutureWatcher<QPixmap> *watcher = new QFutureWatcher<QPixmap>();
    preloadWatchers[index] = watcher;

    connect(watcher, &QFutureWatcher<QPixmap>::finished, this, [this, index, watcher]() {
        if (watcher->isCanceled()) {
            return;
        }
        QPixmap pixmap = watcher->result();
        if (!pixmap.isNull()) {
            QMutexLocker locker(&cacheMutex);
            pixmapCache[index] = pixmap;
        }
        preloadWatchers.remove(index);
        watcher->deleteLater();
    });

    // 预加载时不缩放，存储原始尺寸，显示时再缩放
    QFuture<QPixmap> future = QtConcurrent::run([filePath]() {
        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        QImage image = reader.read();
        if (!image.isNull()) {
            return QPixmap::fromImage(image);
        }
        return QPixmap();
    });

    watcher->setFuture(future);
}

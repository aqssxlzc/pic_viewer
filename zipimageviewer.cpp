#include "zipimageviewer.h"
#include <QVBoxLayout>
#include <QImageReader>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFileInfo>
#include <QtConcurrent>
#include <QBuffer>
#include <QUrl>
#include <QDebug>

ZipImageViewer::ZipImageViewer(const QString &filePath,
                               const QList<ZipReader::ZipEntry> &entries,
                               QSharedPointer<ZipReader> zipReader,
                               QWidget *parent)
    : QDialog(parent)
    , mediaEntries(entries)
    , zipReader(zipReader)
    , currentIndex(0)
    , isVideo(false)
    , currentLabelIndex(0)
    , videoPlayTimer(nullptr)
    , videoBuffer(nullptr)
    , audioOutput(nullptr)
{
    // 找到当前文件在列表中的索引
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i].filePath == filePath) {
            currentIndex = i;
            break;
        }
    }

    setupUI();

    // 先设置全屏，确保 size() 返回正确的全屏尺寸
    setWindowState(Qt::WindowFullScreen);
    setModal(true);

    // 全屏后再加载媒体
    loadMedia();
}

ZipImageViewer::~ZipImageViewer()
{
    // 取消预加载
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

    if (videoBuffer) {
        videoBuffer->close();
        delete videoBuffer;
    }
}

void ZipImageViewer::setupUI()
{
    setStyleSheet("background-color: rgba(0, 0, 0, 240);");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 双缓冲
    imageStack = new QStackedWidget(this);
    mainLayout->addWidget(imageStack);

    for (int i = 0; i < 3; ++i) {
        QLabel *label = new QLabel(imageStack);
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(false);
        label->setStyleSheet("background-color: transparent;");
        imageLabels.append(label);
        imageStack->addWidget(label);
    }
    imageStack->setCurrentIndex(0);

    // 视频播放器
    videoWidget = new QVideoWidget(this);
    videoWidget->setStyleSheet("background-color: black;");
    videoWidget->hide();
    mainLayout->addWidget(videoWidget);

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setVideoOutput(videoWidget);
    mediaPlayer->setAudioOutput(audioOutput);

    videoPlayTimer = new QTimer(this);
    videoPlayTimer->setSingleShot(true);
    connect(videoPlayTimer, &QTimer::timeout, this, &ZipImageViewer::playPendingVideo);

    // 信息标签
    infoLabel = new QLabel(this);
    infoLabel->setStyleSheet(
        "background-color: rgba(0, 0, 0, 150);"
        "color: white;"
        "padding: 8px 15px;"
        "border-radius: 5px;"
        "font-size: 14px;"
    );
    infoLabel->hide();

    // 关闭按钮
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

    // 上一张
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
    connect(prevButton, &QPushButton::clicked, this, &ZipImageViewer::showPrevious);

    // 下一张
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
    connect(nextButton, &QPushButton::clicked, this, &ZipImageViewer::showNext);

    // 播放/暂停按钮
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

    // 淡入动画
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);

    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void ZipImageViewer::loadMedia()
{
    if (currentIndex < 0 || currentIndex >= mediaEntries.size()) {
        return;
    }

    ZipReader::ZipEntry entry = mediaEntries[currentIndex];
    bool wasVideo = isVideo;
    isVideo = isVideoFile(entry.fileName);

    videoPlayTimer->stop();
    pendingVideoData.clear();

    if (wasVideo || isVideo) {
        mediaPlayer->stop();
        mediaPlayer->setSource(QUrl());
    }

    if (isVideo) {
        displayVideo(currentIndex);
    } else {
        displayImage(currentIndex);
        preloadAdjacent();
    }

    QString fileType = isVideo ? "视频" : "图片";
    QString zipName = QFileInfo(zipReader->zipPath()).fileName();
    setWindowTitle(QString("%1查看器 - %2 [%3/%4]")
                   .arg(fileType).arg(zipName)
                   .arg(currentIndex + 1).arg(mediaEntries.size()));

    // 更新信息标签
    infoLabel->setText(QString("%1/%2 - %3")
                       .arg(currentIndex + 1)
                       .arg(mediaEntries.size())
                       .arg(entry.fileName));
    infoLabel->adjustSize();
    infoLabel->move(20, 20);
    infoLabel->show();
}

void ZipImageViewer::displayImage(int index)
{
    videoWidget->hide();
    playPauseButton->hide();
    imageStack->show();

    ZipReader::ZipEntry entry = mediaEntries[index];

    // 检查缓存（缓存的是原始尺寸，需要缩放显示）
    {
        QMutexLocker locker(&cacheMutex);
        if (pixmapCache.contains(index)) {
            currentPixmap = pixmapCache[index];
            updateImage();
            return;
        }
    }

    // 同步加载
    QString errorString;
    QByteArray data = zipReader->readFile(entry.filePath, &errorString);
    if (data.isEmpty()) {
        qWarning() << "无法读取文件:" << entry.filePath << errorString;
        return;
    }

    QImage image;
    if (image.loadFromData(data)) {
        currentPixmap = QPixmap::fromImage(image);
        updateImage();
    }
}

void ZipImageViewer::displayVideo(int index)
{
    ZipReader::ZipEntry entry = mediaEntries[index];

    imageStack->hide();
    videoWidget->show();
    playPauseButton->show();
    playPauseButton->setText("⏸");

    // 读取视频数据到内存
    QString errorString;
    pendingVideoData = zipReader->readFile(entry.filePath, &errorString);
    if (pendingVideoData.isEmpty()) {
        qWarning() << "无法读取视频:" << entry.filePath << errorString;
        return;
    }

    videoPlayTimer->start(100);
}

void ZipImageViewer::playPendingVideo()
{
    if (pendingVideoData.isEmpty()) {
        return;
    }

    // 清理旧的 buffer
    if (videoBuffer) {
        videoBuffer->close();
        delete videoBuffer;
    }

    videoBuffer = new QBuffer(&pendingVideoData);
    videoBuffer->open(QIODevice::ReadOnly);

    mediaPlayer->setSourceDevice(videoBuffer, QUrl());

    disconnect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, nullptr, nullptr);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
        }
    }, Qt::UniqueConnection);

    mediaPlayer->play();
}

bool ZipImageViewer::isVideoFile(const QString &fileName) const
{
    return ZipReader::isVideoFile(fileName);
}

void ZipImageViewer::updateImage()
{
    if (currentPixmap.isNull()) {
        return;
    }

    // 使用屏幕尺寸，而不是窗口尺寸（窗口可能还没有完全显示）
    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen ? screen->geometry().size() : size();

    QPixmap scaledPixmap = currentPixmap.scaled(
        screenSize * 0.9,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    QLabel *label = getAvailableLabel();
    label->setPixmap(scaledPixmap);
    switchToLabel(label);
}

QLabel* ZipImageViewer::getAvailableLabel()
{
    int nextIndex = (currentLabelIndex + 1) % imageLabels.size();
    return imageLabels[nextIndex];
}

void ZipImageViewer::switchToLabel(QLabel *label)
{
    int index = imageLabels.indexOf(label);
    if (index >= 0) {
        imageStack->setCurrentIndex(index);
        currentLabelIndex = index;
    }
}

void ZipImageViewer::showPrevious()
{
    if (mediaEntries.isEmpty()) {
        return;
    }

    currentIndex--;
    if (currentIndex < 0) {
        currentIndex = mediaEntries.size() - 1;
    }

    loadMedia();
}

void ZipImageViewer::showNext()
{
    if (mediaEntries.isEmpty()) {
        return;
    }

    currentIndex++;
    if (currentIndex >= mediaEntries.size()) {
        currentIndex = 0;
    }

    loadMedia();
}

void ZipImageViewer::keyPressEvent(QKeyEvent *event)
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
            currentIndex = mediaEntries.size() - 1;
            loadMedia();
            break;
        case Qt::Key_PageUp:
            currentIndex = qMax(0, currentIndex - 10);
            loadMedia();
            break;
        case Qt::Key_PageDown:
            currentIndex = qMin(mediaEntries.size() - 1, currentIndex + 10);
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

void ZipImageViewer::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    closeButton->move(width() - 70, 20);
    prevButton->move(20, height() / 2 - 30);
    nextButton->move(width() - 80, height() / 2 - 30);
    playPauseButton->move(width() / 2 - 30, height() - 100);

    if (!isVideo && !currentPixmap.isNull()) {
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

    QMutexLocker locker(&cacheMutex);
    pixmapCache.clear();
}

// ==================== 预加载相关方法 ====================

void ZipImageViewer::preloadAdjacent()
{
    // 取消未完成的预加载
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

    // 预加载相邻图片
    for (int offset = -PRELOAD_RANGE; offset <= PRELOAD_RANGE; offset++) {
        if (offset == 0) continue;

        int preloadIndex = currentIndex + offset;
        if (preloadIndex < 0 || preloadIndex >= mediaEntries.size()) continue;

        QString fileName = mediaEntries[preloadIndex].fileName;
        if (!isVideoFile(fileName)) {
            QMutexLocker locker(&cacheMutex);
            if (pixmapCache.contains(preloadIndex)) {
                continue;
            }
            startPreload(preloadIndex);
        }
    }
}

void ZipImageViewer::startPreload(int index)
{
    if (index < 0 || index >= mediaEntries.size()) return;

    QString filePath = mediaEntries[index].filePath;
    QSharedPointer<ZipReader> reader = zipReader;

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
    QFuture<QPixmap> future = QtConcurrent::run([filePath, reader]() -> QPixmap {
        QString errorString;
        QByteArray data = reader->readFile(filePath, &errorString);
        if (data.isEmpty()) {
            return QPixmap();
        }

        QImage image;
        if (image.loadFromData(data)) {
            return QPixmap::fromImage(image);
        }
        return QPixmap();
    });

    watcher->setFuture(future);
}

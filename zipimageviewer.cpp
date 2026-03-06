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
    , currentZipIndex(-1)
    , audioOutput(nullptr)
    , dualMode(DualDisplayMode::Single)
    , dualWidget(nullptr)
    , dualLabelLeft(nullptr)
    , dualLabelRight(nullptr)
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

    // 双图显示容器
    dualWidget = new QWidget(this);
    QHBoxLayout *dualLayout = new QHBoxLayout(dualWidget);
    dualLayout->setContentsMargins(0, 0, 0, 0);
    dualLayout->setSpacing(0);

    dualLabelLeft = new QLabel(dualWidget);
    dualLabelLeft->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dualLabelLeft->setScaledContents(false);
    dualLabelLeft->setStyleSheet("background-color: transparent; margin: 0; padding: 0;");

    dualLabelRight = new QLabel(dualWidget);
    dualLabelRight->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dualLabelRight->setScaledContents(false);
    dualLabelRight->setStyleSheet("background-color: transparent; margin: 0; padding: 0;");

    dualLayout->addWidget(dualLabelLeft, 1);
    dualLayout->addWidget(dualLabelRight, 1);
    dualWidget->hide();
    mainLayout->addWidget(dualWidget);

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

    // 根据双图模式选择显示方式
    if (dualMode == DualDisplayMode::Single) {
        imageStack->show();
        dualWidget->hide();
    } else {
        imageStack->hide();
        dualWidget->show();
    }

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
        // 获取目标尺寸并缩放
        QScreen *screen = QApplication::primaryScreen();
        QSize targetSize = screen ? screen->geometry().size() : size();
        if (image.width() > targetSize.width() || image.height() > targetSize.height()) {
            QSize scaledSize = image.size().scaled(targetSize, Qt::KeepAspectRatio);
            image = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
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

    // 直接显示，缩放已在加载或预加载时完成
    QLabel *label = getAvailableLabel();
    label->setPixmap(currentPixmap);
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

    // 双图模式下每次跳过两张
    if (dualMode != DualDisplayMode::Single) {
        currentIndex -= 2;
        if (currentIndex < 0) {
            currentIndex += mediaEntries.size();
        }
    } else {
        currentIndex--;
        if (currentIndex < 0) {
            currentIndex = mediaEntries.size() - 1;
        }
    }

    loadMedia();

    // 双图模式下更新显示
    if (dualMode != DualDisplayMode::Single && !isVideo) {
        if (loadDualImages()) {
            updateDualImages();
        }
    }
}

void ZipImageViewer::showNext()
{
    if (mediaEntries.isEmpty()) {
        return;
    }

    // 双图模式下每次跳过两张
    if (dualMode != DualDisplayMode::Single) {
        currentIndex += 2;
        if (currentIndex >= mediaEntries.size()) {
            currentIndex -= mediaEntries.size();
        }
    } else {
        currentIndex++;
        if (currentIndex >= mediaEntries.size()) {
            currentIndex = 0;
        }
    }

    loadMedia();

    // 双图模式下更新显示
    if (dualMode != DualDisplayMode::Single && !isVideo) {
        if (loadDualImages()) {
            updateDualImages();
        }
    }
}

void ZipImageViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
        case Qt::Key_Q:
            close();
            break;
        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_W:
            showPrevious();
            break;
        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_S:
            showNext();
            break;
        case Qt::Key_A:
            // 切换到前一个 ZIP 文件
            if (!adjacentZipFiles.isEmpty() && currentZipIndex > 0) {
                emit switchToZip(adjacentZipFiles[currentZipIndex - 1]);
                close();
            }
            break;
        case Qt::Key_D:
            // 切换到后一个 ZIP 文件
            if (!adjacentZipFiles.isEmpty() && currentZipIndex < adjacentZipFiles.size() - 1) {
                emit switchToZip(adjacentZipFiles[currentZipIndex + 1]);
                close();
            }
            break;
        case Qt::Key_E:
            // 切换双图显示模式
            if (!isVideo) {
                toggleDualDisplayMode();
            }
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
        // 窗口大小变化时重新缩放当前图片
        QScreen *screen = QApplication::primaryScreen();
        QSize targetSize = screen ? screen->geometry().size() : size();
        
        // 重新加载当前图片
        if (!mediaEntries.isEmpty() && currentIndex >= 0 && currentIndex < mediaEntries.size()) {
            QString errorString;
            QByteArray data = zipReader->readFile(mediaEntries[currentIndex].filePath, &errorString);
            if (!data.isEmpty()) {
                QImage image;
                if (image.loadFromData(data)) {
                    if (image.width() > targetSize.width() || image.height() > targetSize.height()) {
                        QSize scaledSize = image.size().scaled(targetSize, Qt::KeepAspectRatio);
                        image = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    currentPixmap = QPixmap::fromImage(image);
                    QLabel *currentLabel = imageLabels[currentLabelIndex];
                    currentLabel->setPixmap(currentPixmap);
                }
            }
        }
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

    // 获取目标尺寸用于预缩放
    QScreen *screen = QApplication::primaryScreen();
    QSize targetSize = screen ? screen->geometry().size() : QSize(1920, 1080);

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

    // 在后台线程预先缩放到目标尺寸
    QFuture<QPixmap> future = QtConcurrent::run([filePath, reader, targetSize]() -> QPixmap {
        QString errorString;
        QByteArray data = reader->readFile(filePath, &errorString);
        if (data.isEmpty()) {
            return QPixmap();
        }

        QImage image;
        if (image.loadFromData(data)) {
            // 预先缩放
            if (image.width() > targetSize.width() || image.height() > targetSize.height()) {
                QSize scaledSize = image.size().scaled(targetSize, Qt::KeepAspectRatio);
                image = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            return QPixmap::fromImage(image);
        }
        return QPixmap();
    });

    watcher->setFuture(future);
}

void ZipImageViewer::setAdjacentZipFiles(const QStringList &zipFiles, int index)
{
    adjacentZipFiles = zipFiles;
    currentZipIndex = index;
}

QString ZipImageViewer::currentZipPath() const
{
    if (currentZipIndex >= 0 && currentZipIndex < adjacentZipFiles.size()) {
        return adjacentZipFiles[currentZipIndex];
    }
    return QString();
}

// ==================== 双图显示相关方法 ====================

void ZipImageViewer::toggleDualDisplayMode()
{
    switch (dualMode) {
        case DualDisplayMode::Single:
            dualMode = DualDisplayMode::DualNextLeft;
            break;
        case DualDisplayMode::DualNextLeft:
            dualMode = DualDisplayMode::DualNextRight;
            break;
        case DualDisplayMode::DualNextRight:
            dualMode = DualDisplayMode::Single;
            break;
    }

    if (dualMode == DualDisplayMode::Single) {
        // 恢复单图显示
        dualWidget->hide();
        imageStack->show();
        updateImage();
    } else {
        // 尝试双图显示
        if (loadDualImages()) {
            imageStack->hide();
            dualWidget->show();
            updateDualImages();
        } else {
            // 无法双图显示，恢复单图
            dualMode = DualDisplayMode::Single;
        }
    }
}

int ZipImageViewer::findNextImageIndex(int fromIndex) const
{
    for (int i = fromIndex + 1; i < mediaEntries.size(); ++i) {
        if (!isVideoFile(mediaEntries[i].fileName)) {
            return i;
        }
    }
    // 循环查找
    for (int i = 0; i < fromIndex; ++i) {
        if (!isVideoFile(mediaEntries[i].fileName)) {
            return i;
        }
    }
    return -1;
}

bool ZipImageViewer::canDisplayDual() const
{
    if (currentPixmap.isNull()) return false;

    int nextIdx = findNextImageIndex(currentIndex);
    if (nextIdx < 0) return false;

    return true;
}

bool ZipImageViewer::loadDualImages()
{
    if (currentPixmap.isNull()) return false;

    int nextIdx = findNextImageIndex(currentIndex);
    if (nextIdx < 0 || nextIdx == currentIndex) return false;

    // 加载下一张图片
    ZipReader::ZipEntry entry = mediaEntries[nextIdx];

    // 检查缓存
    {
        QMutexLocker locker(&cacheMutex);
        if (pixmapCache.contains(nextIdx)) {
            nextPixmap = pixmapCache[nextIdx];
            return true;
        }
    }

    // 同步加载
    QString errorString;
    QByteArray data = zipReader->readFile(entry.filePath, &errorString);
    if (data.isEmpty()) {
        return false;
    }

    QImage image;
    if (image.loadFromData(data)) {
        QScreen *screen = QApplication::primaryScreen();
        QSize targetSize = screen ? screen->geometry().size() : size();
        // 双图模式下每张图片只占用一半宽度
        targetSize.setWidth(targetSize.width() / 2 - 10);
        if (image.width() > targetSize.width() || image.height() > targetSize.height()) {
            QSize scaledSize = image.size().scaled(targetSize, Qt::KeepAspectRatio);
            image = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        nextPixmap = QPixmap::fromImage(image);
        return true;
    }

    return false;
}

void ZipImageViewer::updateDualImages()
{
    if (currentPixmap.isNull() || nextPixmap.isNull()) return;

    QPixmap leftPixmap, rightPixmap;

    switch (dualMode) {
        case DualDisplayMode::DualNextLeft:
            // 当前在左，下一张在右（阅读顺序：先看左边的当前图，再看右边的下一张）
            leftPixmap = currentPixmap;
            rightPixmap = nextPixmap;
            break;
        case DualDisplayMode::DualNextRight:
            // 下一张在左，当前在右（阅读顺序：先看右边的当前图，再看左边的下一张）
            leftPixmap = nextPixmap;
            rightPixmap = currentPixmap;
            break;
        default:
            return;
    }

    // 获取可用尺寸（屏幕的一半）
    QScreen *screen = QApplication::primaryScreen();
    QSize targetSize = screen ? screen->geometry().size() : size();
    // 精确计算每张图片的最大尺寸（屏幕宽度的一半）
    targetSize.setWidth(targetSize.width() / 2);

    // 缩放图片以适应半屏
    QPixmap scaledLeft = leftPixmap;
    QPixmap scaledRight = rightPixmap;

    if (leftPixmap.width() > targetSize.width() || leftPixmap.height() > targetSize.height()) {
        QSize scaledSize = leftPixmap.size().scaled(targetSize, Qt::KeepAspectRatio);
        scaledLeft = leftPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (rightPixmap.width() > targetSize.width() || rightPixmap.height() > targetSize.height()) {
        QSize scaledSize = rightPixmap.size().scaled(targetSize, Qt::KeepAspectRatio);
        scaledRight = rightPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    dualLabelLeft->setPixmap(scaledLeft);
    dualLabelRight->setPixmap(scaledRight);

    // 更新信息标签 - 显示两张图片的信息
    int nextIdx = findNextImageIndex(currentIndex);
    ZipReader::ZipEntry entry = mediaEntries[currentIndex];
    ZipReader::ZipEntry nextEntry = mediaEntries[nextIdx];
    infoLabel->setText(QString("%1/%2 - %3 | %4/%5 - %6")
                       .arg(currentIndex + 1)
                       .arg(mediaEntries.size())
                       .arg(entry.fileName)
                       .arg(nextIdx + 1)
                       .arg(mediaEntries.size())
                       .arg(nextEntry.fileName));
    infoLabel->adjustSize();
    infoLabel->move(20, 20);
    infoLabel->show();
}

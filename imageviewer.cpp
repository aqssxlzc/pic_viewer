#include "imageviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QMediaContent>
#include <QFileInfo>

ImageViewer::ImageViewer(const QString &filePath, const QStringList &allPaths, QWidget *parent)
    : QDialog(parent)
    , mediaPaths(allPaths)
    , currentIndex(allPaths.indexOf(filePath))
    , isVideo(false)
{
    setupUI();
    loadMedia();
    
    setWindowState(Qt::WindowFullScreen);
    setModal(true);
}

void ImageViewer::setupUI()
{
    setStyleSheet("background-color: rgba(0, 0, 0, 240);");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Image label
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setScaledContents(false);
    mainLayout->addWidget(imageLabel);
    
    // Video widget
    videoWidget = new QVideoWidget(this);
    videoWidget->hide();
    mainLayout->addWidget(videoWidget);
    
    // Media player
    mediaPlayer = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    mediaPlayer->setVideoOutput(videoWidget);

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
        if (mediaPlayer->state() == QMediaPlayer::PlayingState) {
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
    isVideo = isVideoFile(filePath);
    
    if (isVideo) {
        // Stop current video if playing
        mediaPlayer->stop();
        
        // Show video widget, hide image label
        imageLabel->hide();
        videoWidget->show();
        playPauseButton->show();
        
        // Load and play video
        mediaPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(filePath)));
        
        // Loop video
        connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                mediaPlayer->setPosition(0);
                mediaPlayer->play();
            }
        });
        
        mediaPlayer->play();
        playPauseButton->setText("⏸");
        updateVideo();
    } else {
        // Stop video if playing
        mediaPlayer->stop();
        
        // Show image label, hide video widget
        videoWidget->hide();
        imageLabel->show();
        playPauseButton->hide();
        
        // Load image
        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        
        QImage image = reader.read();
        if (!image.isNull()) {
            currentPixmap = QPixmap::fromImage(image);
            updateImage();
        }
    }

    QString fileType = isVideo ? "视频" : "图片";
    setWindowTitle(QString("%1查看器 - %2/%3").arg(fileType).arg(currentIndex + 1).arg(mediaPaths.size()));
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

    QSize screenSize = size();
    
    // Scale image to fit screen while maintaining aspect ratio
    QPixmap scaledPixmap = currentPixmap.scaled(
        screenSize * 0.9,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    
    imageLabel->setPixmap(scaledPixmap);
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
            // Jump to first media
            currentIndex = 0;
            loadMedia();
            break;
        case Qt::Key_End:
            // Jump to last media
            currentIndex = mediaPaths.size() - 1;
            loadMedia();
            break;
        case Qt::Key_PageUp:
            // Jump 10 media back
            currentIndex = qMax(0, currentIndex - 10);
            loadMedia();
            break;
        case Qt::Key_PageDown:
            // Jump 10 media forward
            currentIndex = qMin(mediaPaths.size() - 1, currentIndex + 10);
            loadMedia();
            break;
        case Qt::Key_Space:
            // Play/Pause video with spacebar
            if (isVideo) {
                if (mediaPlayer->state() == QMediaPlayer::PlayingState) {
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
    } else {
        updateImage();
    }
}

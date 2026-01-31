#include "mediaitem.h"
#include <QVBoxLayout>
#include <QImageReader>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QMediaContent>
#include <QPainterPath>
#include <QTimer>
#include <QtConcurrent>

MediaItem::MediaItem(const QFileInfo &fileInfo, QWidget *parent)
    : QWidget(parent)
    , fileInfo(fileInfo)
    , thumbnailLabel(nullptr)
    , fileNameLabel(nullptr)
    , playIconLabel(nullptr)
    , mediaPlayer(nullptr)
    , videoWidget(nullptr)
    , loaded(false)
    , currentScale(1.0)
    , isActive(false)
    , videoLoaded(false)
{
    QString ext = fileInfo.suffix().toLower();
    QStringList imageExts;
    imageExts << "jpg" << "jpeg" << "png" << "gif" << "bmp" << "webp";
    QStringList videoExts;
    videoExts << "mp4" << "avi" << "mkv" << "mov" << "wmv" << "flv" << "webm";
    
    isImageFile = imageExts.contains(ext);
    isVideoFile = videoExts.contains(ext);

    setStyleSheet("background-color: #2a2a2a; border-radius: 12px;");
    setCursor(Qt::PointingHandCursor);
    
    // Enable hardware acceleration
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (isImageFile) {
        thumbnailLabel = new QLabel(this);
        thumbnailLabel->setScaledContents(false);
        thumbnailLabel->setAlignment(Qt::AlignCenter);
        thumbnailLabel->setStyleSheet("background-color: transparent;");
        layout->addWidget(thumbnailLabel);
    } else if (isVideoFile) {
        setupVideoPlayer();
        layout->addWidget(videoWidget);
        
        playIconLabel = new QLabel("▶", this);
        playIconLabel->setStyleSheet(
            "background-color: rgba(0, 0, 0, 0.7);"
            "color: white;"
            "border-radius: 30px;"
            "font-size: 30px;"
            "padding: 15px 20px;"
        );
        playIconLabel->setAlignment(Qt::AlignCenter);
        playIconLabel->setFixedSize(60, 60);
        playIconLabel->move(width()/2 - 30, height()/2 - 30);
    }

    fileNameLabel = new QLabel(fileInfo.fileName(), this);
    fileNameLabel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 transparent, stop:1 rgba(0, 0, 0, 200));"
        "color: white;"
        "font-size: 11px;"
        "padding: 10px;"
    );
    fileNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    fileNameLabel->setWordWrap(false);
    
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(fileNameLabel);
    effect->setOpacity(0.9);
    fileNameLabel->setGraphicsEffect(effect);

    // Setup hover animation
    scaleAnimation = new QPropertyAnimation(this, "");
    scaleAnimation->setDuration(200);
    scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

    imageWatcher = new QFutureWatcher<QImage>(this);
    imageLoading = false;
    connect(imageWatcher, &QFutureWatcher<QImage>::finished, this, [this]() {
        imageLoading = false;
        if (!isActive || !thumbnailLabel) {
            return;
        }
        QImage image = imageWatcher->result();
        if (!image.isNull()) {
            originalPixmap = QPixmap::fromImage(image);
            thumbnailLabel->setPixmap(originalPixmap.scaled(
                thumbnailLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            ));
            loaded = true;
        }
    });
}

MediaItem::~MediaItem()
{
    if (mediaPlayer) {
        mediaPlayer->stop();
        delete mediaPlayer;
    }
}

void MediaItem::setupVideoPlayer()
{
    videoWidget = new QVideoWidget(this);
    videoWidget->setStyleSheet("background-color: #2a2a2a;");
    
    mediaPlayer = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    mediaPlayer->setVideoOutput(videoWidget);
    mediaPlayer->setMuted(true);

    // Loop video
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
        }
    });

    // Handle codec errors gracefully
    connect(mediaPlayer, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, [this](QMediaPlayer::Error) {
        if (playIconLabel) {
            playIconLabel->setStyleSheet(
                "background-color: rgba(200, 0, 0, 0.7);"
                "color: white;"
                "border-radius: 30px;"
                "font-size: 20px;"
                "padding: 15px 20px;"
            );
            playIconLabel->setText("✗");
        }
    });
}

void MediaItem::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!loaded) {
        loadMedia();
    }
}

void MediaItem::loadMedia()
{
    if (isImageFile) {
        if (!isActive || imageLoading || loaded) {
            return;
        }
        imageLoading = true;
        const QString path = fileInfo.absoluteFilePath();
        imageWatcher->setFuture(QtConcurrent::run([path]() -> QImage {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            QSize imageSize = reader.size();
            if (imageSize.width() > 500 || imageSize.height() > 500) {
                imageSize.scale(500, 500, Qt::KeepAspectRatio);
                reader.setScaledSize(imageSize);
            }
            return reader.read();
        }));
    } else if (isVideoFile && mediaPlayer) {
        if (!videoLoaded) {
            QUrl videoUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
            mediaPlayer->setMedia(QMediaContent(videoUrl));
            videoLoaded = true;
        }
        if (isActive) {
            mediaPlayer->play();
        }
    }
}

void MediaItem::setActive(bool active)
{
    if (isActive == active) {
        return;
    }
    isActive = active;

    if (!isVideoFile || !mediaPlayer) {
        if (isImageFile && isActive && !loaded && !imageLoading) {
            loadMedia();
        }
        return;
    }

    if (isActive) {
        if (!videoLoaded) {
            loadMedia();
        }
        QTimer::singleShot(0, mediaPlayer, &QMediaPlayer::play);
    } else {
        // Stop and unload to free decoder resources
        QTimer::singleShot(0, mediaPlayer, &QMediaPlayer::stop);
        QTimer::singleShot(0, this, [this]() {
            mediaPlayer->setMedia(QMediaContent());
            videoLoaded = false;
        });
    }
}

void MediaItem::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    currentScale = 1.05;
    update();
    
    // Add shadow effect
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setXOffset(0);
    shadow->setYOffset(5);
    shadow->setColor(QColor(0, 0, 0, 180));
    setGraphicsEffect(shadow);
}

void MediaItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    currentScale = 1.0;
    update();
    setGraphicsEffect(nullptr);
}

void MediaItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    // Emit clicked for both images and videos
    emit clicked();
}

void MediaItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Apply scale transform
    if (currentScale != 1.0) {
        painter.translate(width() / 2.0, height() / 2.0);
        painter.scale(currentScale, currentScale);
        painter.translate(-width() / 2.0, -height() / 2.0);
    }
    
    // Draw rounded rectangle background
    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);
    painter.setClipPath(path);
    painter.fillRect(rect(), QColor("#2a2a2a"));
}

void MediaItem::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    if (fileNameLabel) {
        fileNameLabel->setGeometry(0, height() - 40, width(), 40);
    }
    
    if (playIconLabel) {
        playIconLabel->move(width()/2 - 30, height()/2 - 30);
    }
    
    if (isImageFile && thumbnailLabel && !originalPixmap.isNull()) {
        thumbnailLabel->setPixmap(originalPixmap.scaled(
            thumbnailLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
}

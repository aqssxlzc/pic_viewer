#include "zipmediaitem.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QTimer>
#include <QtConcurrent>

ZipMediaItem::ZipMediaItem(const ZipReader::ZipEntry &entry,
                           QSharedPointer<ZipReader> zipReader,
                           QWidget *parent)
    : QWidget(parent)
    , m_entry(entry)
    , m_zipReader(zipReader)
    , thumbnailLabel(nullptr)
    , fileNameLabel(nullptr)
    , playIconLabel(nullptr)
    , loaded(false)
    , currentScale(1.0)
    , isActive(false)
    , imageLoading(false)
{
    isImageFile = ZipReader::isImageFile(entry.fileName);
    isVideoFile = ZipReader::isVideoFile(entry.fileName);

    setStyleSheet("background-color: #2a2a2a; border-radius: 12px;");
    setCursor(Qt::PointingHandCursor);

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
        QLabel *placeholder = new QLabel(this);
        placeholder->setStyleSheet("background-color: #1a1a1a;");
        placeholder->setAlignment(Qt::AlignCenter);
        layout->addWidget(placeholder);

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

    fileNameLabel = new QLabel(entry.fileName, this);
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

    scaleAnimation = new QPropertyAnimation(this, "");
    scaleAnimation->setDuration(200);
    scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

    imageWatcher = new QFutureWatcher<QImage>(this);
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

ZipMediaItem::~ZipMediaItem()
{
}

void ZipMediaItem::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!loaded && isImageFile) {
        loadMedia();
    }
}

void ZipMediaItem::loadMedia()
{
    if (!isImageFile || !isActive || imageLoading || loaded || !m_zipReader) {
        return;
    }

    imageLoading = true;
    QString filePath = m_entry.filePath;
    QSharedPointer<ZipReader> zipReader = m_zipReader;

    imageWatcher->setFuture(QtConcurrent::run([filePath, zipReader]() -> QImage {
        QString errorString;
        QByteArray data = zipReader->readFile(filePath, &errorString);
        if (data.isEmpty()) {
            return QImage();
        }

        QImage image;
        if (image.loadFromData(data)) {
            // 缩放以节省内存
            if (image.width() > 500 || image.height() > 500) {
                image = image.scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
        return image;
    }));
}

void ZipMediaItem::setActive(bool active)
{
    if (isActive == active) {
        return;
    }
    isActive = active;

    if (isImageFile && isActive && !loaded && !imageLoading) {
        loadMedia();
    }
}

void ZipMediaItem::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    currentScale = 1.05;
    update();

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setXOffset(0);
    shadow->setYOffset(5);
    shadow->setColor(QColor(0, 0, 0, 180));
    setGraphicsEffect(shadow);
}

void ZipMediaItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    currentScale = 1.0;
    update();
    setGraphicsEffect(nullptr);
}

void ZipMediaItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit clicked();
}

void ZipMediaItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (currentScale != 1.0) {
        painter.translate(width() / 2.0, height() / 2.0);
        painter.scale(currentScale, currentScale);
        painter.translate(-width() / 2.0, -height() / 2.0);
    }

    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);
    painter.setClipPath(path);
    painter.fillRect(rect(), QColor("#2a2a2a"));
}

void ZipMediaItem::resizeEvent(QResizeEvent *event)
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

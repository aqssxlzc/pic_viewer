#ifndef ZIPMEDIAITEM_H
#define ZIPMEDIAITEM_H

#include <QWidget>
#include <QLabel>
#include <QFileInfo>
#include <QPixmap>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFutureWatcher>
#include <QImage>
#include <QMutex>
#include "zipreader.h"

/**
 * ZIP 压缩包内媒体文件的缩略图组件
 */
class ZipMediaItem : public QWidget
{
    Q_OBJECT

public:
    explicit ZipMediaItem(const ZipReader::ZipEntry &entry,
                          QSharedPointer<ZipReader> zipReader,
                          QWidget *parent = nullptr);
    ~ZipMediaItem();
    void setActive(bool active);

    QString filePath() const { return m_entry.filePath; }
    QString fileName() const { return m_entry.fileName; }

signals:
    void clicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadMedia();
    bool isImage() const;
    bool isVideo() const;

    ZipReader::ZipEntry m_entry;
    QSharedPointer<ZipReader> m_zipReader;
    QLabel *thumbnailLabel;
    QLabel *fileNameLabel;
    QLabel *playIconLabel;
    QPropertyAnimation *scaleAnimation;
    QFutureWatcher<QImage> *imageWatcher;
    bool imageLoading;
    bool loaded;
    bool isImageFile;
    bool isVideoFile;
    bool isActive;

    qreal currentScale;
    QPixmap originalPixmap;
};

#endif // ZIPMEDIAITEM_H

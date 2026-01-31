#ifndef MEDIAITEM_H
#define MEDIAITEM_H

#include <QWidget>
#include <QLabel>
#include <QFileInfo>
#include <QPixmap>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class MediaItem : public QWidget
{
    Q_OBJECT

public:
    explicit MediaItem(const QFileInfo &fileInfo, QWidget *parent = nullptr);
    ~MediaItem();
    void setActive(bool active);

signals:
    void clicked();

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadMedia();
    void setupVideoPlayer();
    bool isImage() const;
    bool isVideo() const;

    QFileInfo fileInfo;
    QLabel *thumbnailLabel;
    QLabel *fileNameLabel;
    QLabel *playIconLabel;
    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QPropertyAnimation *scaleAnimation;
    
    bool loaded;
    bool isImageFile;
    bool isVideoFile;
    bool isActive;
    
    qreal currentScale;
    QPixmap originalPixmap;
};

#endif // MEDIAITEM_H

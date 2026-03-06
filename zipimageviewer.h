#ifndef ZIPIMAGEVIEWER_H
#define ZIPIMAGEVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QStringList>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QMap>
#include <QFutureWatcher>
#include <QImage>
#include <QStackedWidget>
#include <QMutex>
#include <QTimer>
#include <QBuffer>
#include "zipreader.h"

/**
 * ZIP 压缩包内媒体文件的查看器
 * 支持图片和视频浏览，支持左右键切换
 */
class ZipImageViewer : public QDialog
{
    Q_OBJECT

public:
    explicit ZipImageViewer(const QString &filePath,
                           const QList<ZipReader::ZipEntry> &entries,
                           QSharedPointer<ZipReader> zipReader,
                           QWidget *parent = nullptr);
    ~ZipImageViewer();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showPrevious();
    void showNext();
    void playPendingVideo();

private:
    void loadMedia();
    void setupUI();
    void updateImage();
    bool isVideoFile(const QString &fileName) const;
    void displayImage(int index);
    void displayVideo(int index);

    // 预加载
    void preloadAdjacent();
    void startPreload(int index);

    // 双缓冲
    QLabel* getAvailableLabel();
    void switchToLabel(QLabel *label);

    QStackedWidget *imageStack;
    QList<QLabel*> imageLabels;
    int currentLabelIndex;

    QVideoWidget *videoWidget;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *closeButton;
    QPushButton *playPauseButton;
    QLabel *infoLabel;

    QList<ZipReader::ZipEntry> mediaEntries;
    QSharedPointer<ZipReader> zipReader;
    int currentIndex;
    QPixmap currentPixmap;
    bool isVideo;

    // 视频播放
    QByteArray pendingVideoData;
    QTimer *videoPlayTimer;
    QBuffer *videoBuffer;

    // 预加载缓存
    static const int PRELOAD_RANGE = 3;
    QMap<int, QPixmap> pixmapCache;
    QMap<int, QFutureWatcher<QPixmap>*> preloadWatchers;
    QMutex cacheMutex;
};

#endif // ZIPIMAGEVIEWER_H

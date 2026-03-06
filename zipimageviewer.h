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

// 双图显示模式
enum class DualDisplayMode {
    Single,       // 单图显示
    DualNextLeft, // 双图显示，下一张在左
    DualNextRight // 双图显示，下一张在右
};

/**
 * ZIP 压缩包内媒体文件的查看器
 * 支持图片和视频浏览，支持左右键切换
 * A/D 键切换相邻 ZIP 文件
 * E 键切换双图显示模式
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

    // 设置相邻 ZIP 文件列表和当前索引
    void setAdjacentZipFiles(const QStringList &zipFiles, int currentIndex);

    // 获取当前 ZIP 路径
    QString currentZipPath() const;

signals:
    void switchToZip(const QString &zipPath);  // 切换到指定 ZIP

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

    // 双图显示
    void toggleDualDisplayMode();
    bool loadDualImages();
    void updateDualImages();
    int findNextImageIndex(int fromIndex) const;
    bool canDisplayDual() const;

    QStackedWidget *imageStack;
    QList<QLabel*> imageLabels;
    int currentLabelIndex;

    // 双图显示标签
    QWidget *dualWidget;
    QLabel *dualLabelLeft;
    QLabel *dualLabelRight;
    QPixmap nextPixmap;
    DualDisplayMode dualMode;

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
    static const int PRELOAD_RANGE = 50;  // 前后各预加载50张
    QMap<int, QPixmap> pixmapCache;
    QMap<int, QFutureWatcher<QPixmap>*> preloadWatchers;
    QMutex cacheMutex;

    // 相邻 ZIP 文件导航
    QStringList adjacentZipFiles;
    int currentZipIndex;
};

#endif // ZIPIMAGEVIEWER_H

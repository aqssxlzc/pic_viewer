#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QStringList>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QMap>
#include <QFutureWatcher>
#include <QImage>
#include <QStackedWidget>
#include <QMutex>
#include <QTimer>

class ImageViewer : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewer(const QString &filePath, const QStringList &allPaths, QWidget *parent = nullptr);
    ~ImageViewer();

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
    void updateVideo();
    bool isVideoFile(const QString &filePath) const;
    void displayImage(int index);
    void displayVideo(int index);

    // 预加载相关
    void preloadAdjacent();
    void startPreload(int index, const QSize &targetSize);

    // 双缓冲相关
    QLabel* getAvailableLabel();
    void switchToLabel(QLabel *label);

    QStackedWidget *imageStack;
    QList<QLabel*> imageLabels;
    int currentLabelIndex;

    QVideoWidget *videoWidget;
    QMediaPlayer *mediaPlayer;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *closeButton;
    QPushButton *playPauseButton;

    QStringList mediaPaths;
    int currentIndex;
    QPixmap currentPixmap;
    bool isVideo;

    // 视频延迟播放
    QString pendingVideoPath;
    QTimer *videoPlayTimer;

    // 预加载缓存
    static const int PRELOAD_RANGE = 3;
    QMap<int, QPixmap> pixmapCache;
    QMap<int, QFutureWatcher<QPixmap>*> preloadWatchers;
    QMutex cacheMutex;
};

#endif // IMAGEVIEWER_H

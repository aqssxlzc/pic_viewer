#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QStringList>
#include <QMediaPlayer>
#include <QVideoWidget>

class ImageViewer : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewer(const QString &filePath, const QStringList &allPaths, QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showPrevious();
    void showNext();

private:
    void loadMedia();
    void setupUI();
    void updateImage();
    void updateVideo();
    bool isVideoFile(const QString &filePath) const;

    QLabel *imageLabel;
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
};

#endif // IMAGEVIEWER_H

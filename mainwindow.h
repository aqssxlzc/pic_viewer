#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTimer>
#include <QList>
#include <QFileInfo>
#include "imageviewer.h"

class MediaGrid;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFolder();
    void onScrollChanged(int value);
    void checkLoadMore();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void loadMediaFiles(const QString &directory);
    void loadNextBatch();
    void updateStats();

    QScrollArea *scrollArea;
    MediaGrid *mediaGrid;
    QPushButton *openButton;
    QLabel *statsLabel;
    QTimer *loadTimer;
    
    QList<QFileInfo> allFiles;
    QStringList imageExtensions;
    QStringList videoExtensions;
    
    int currentBatch;
    int batchSize;
    int imageCount;
    int videoCount;
    QString lastDirectory;
};

class MediaGrid : public QWidget
{
    Q_OBJECT

public:
    MediaGrid(QWidget *parent = nullptr);
    void addMediaItem(const QFileInfo &fileInfo);
    void clear();
    int itemCount() const { return items.size(); }
    void updateVisibleItems(const QRect &visibleRect);

signals:
    void imageClicked(const QString &filePath, const QStringList &imagePaths);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void relayout();
    
    QList<QWidget*> items;
    int columns;
    int itemSize;
    int spacing;
};

#endif // MAINWINDOW_H

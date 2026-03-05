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
#include <QSharedPointer>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include "imageviewer.h"
#include "zipreader.h"

class MediaGrid;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 用于 MediaGrid 访问 ZIP 条目
    QSharedPointer<ZipReader> getCurrentZipReader() const { return currentZipReader; }
    QList<ZipReader::ZipEntry> getZipEntries() const { return zipEntries; }

private slots:
    void openFolder();
    void openZipFile();
    void onScrollChanged(int value);
    void checkLoadMore();
    void toggleSortOrder();
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onPathEditReturnPressed();
    void goBack();
    void goUp();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupFileTree();
    void setupMediaArea();
    void populateTree(const QString &path);
    void loadMediaFiles(const QString &directory, bool restoreScroll = false);
    void loadZipFile(const QString &zipPath);
    void loadNextBatch();
    void updateStats();
    void clearCurrentView();
    void updateNavigationButtons(const QString &path);
    void addToHistory(const QString &path);
    bool isZipFile(const QString &path) const;

    // 主布局组件
    QSplitter *mainSplitter;
    QWidget *leftPanel;
    QTreeWidget *fileTree;
    QLineEdit *pathEdit;
    QPushButton *backButton;
    QPushButton *upButton;

    QScrollArea *scrollArea;
    MediaGrid *mediaGrid;
    QPushButton *sortButton;
    QLabel *statsLabel;
    QTimer *loadTimer;

    QList<QFileInfo> allFiles;
    QStringList imageExtensions;
    QStringList videoExtensions;

    int currentBatch;
    int batchSize;
    int imageCount;
    int videoCount;
    QString currentPath;
    QStringList navigationHistory;
    int historyIndex;
    bool sortAscending;
    int lastScrollPosition;

    // ZIP 文件支持
    QSharedPointer<ZipReader> currentZipReader;
    QList<ZipReader::ZipEntry> zipEntries;
};

class MediaGrid : public QWidget
{
    Q_OBJECT

public:
    MediaGrid(QWidget *parent = nullptr);
    void addMediaItem(const QFileInfo &fileInfo);
    void addZipMediaItem(const ZipReader::ZipEntry &entry, QSharedPointer<ZipReader> zipReader);
    void addZipFileItem(const QString &zipPath);
    void setZipEntries(const QList<ZipReader::ZipEntry> &entries) { zipEntries = entries; }
    void setZipReader(QSharedPointer<ZipReader> reader) { zipReader = reader; }
    void clear();
    int itemCount() const { return items.size(); }
    void updateVisibleItems(const QRect &visibleRect);

signals:
    void imageClicked(const QString &filePath, const QStringList &imagePaths);
    void zipMediaClicked(const QString &filePath, const QList<ZipReader::ZipEntry> &entries, QSharedPointer<ZipReader> zipReader);
    void zipFileClicked(const QString &zipPath);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void relayout();

    QList<QWidget*> items;
    int columns;
    int itemSize;
    int spacing;

    // ZIP 文件支持
    QList<ZipReader::ZipEntry> zipEntries;
    QSharedPointer<ZipReader> zipReader;
};

#endif // MAINWINDOW_H

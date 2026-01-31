#include "mainwindow.h"
#include "mediaitem.h"
#include <QDir>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentBatch(0)
    , batchSize(30)
    , imageCount(0)
    , videoCount(0)
    , sortAscending(false)
    , lastScrollPosition(0)
{
    imageExtensions << "jpg" << "jpeg" << "png" << "gif" << "bmp" << "webp" << "svg";
    videoExtensions << "mp4" << "avi" << "mkv" << "mov" << "wmv" << "flv" << "webm";
    
    setupUI();
    
    loadTimer = new QTimer(this);
    loadTimer->setSingleShot(true);
    loadTimer->setInterval(150);
    connect(loadTimer, &QTimer::timeout, this, &MainWindow::checkLoadMore);

    // Restore last opened directory
    QSettings settings("MediaViewer", "MediaViewer");
    lastDirectory = settings.value("lastDirectory", "").toString();
    sortAscending = settings.value("sortAscending", false).toBool();
    lastScrollPosition = settings.value("scrollPosition", 0).toInt();
    
    qDebug() << "Restored settings - Directory:" << lastDirectory << "ScrollPos:" << lastScrollPosition << "SortAsc:" << sortAscending;
    
    if (!lastDirectory.isEmpty()) {
        loadMediaFiles(lastDirectory, true);  // restoreScroll = true
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("媒体文件查看器");
    resize(1200, 800);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Header
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: #2b2b2b; padding: 15px;");
    headerWidget->setMinimumHeight(80);
    
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    
    QLabel *titleLabel = new QLabel("📸 媒体文件查看器");
    titleLabel->setStyleSheet("color: white; font-size: 20px; font-weight: bold;");
    
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    openButton = new QPushButton("选择文件夹");
    openButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2563eb;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1d4ed8;"
        "}"
    );
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFolder);
    
    QPushButton *sortButton = new QPushButton("排序: 新到旧");
    sortButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #6b7280;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #4b5563;"
        "}"
    );
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::toggleSortOrder);
    
    statsLabel = new QLabel("未加载文件");
    statsLabel->setStyleSheet("color: #999; font-size: 14px;");
    
    controlsLayout->addWidget(openButton);
    controlsLayout->addWidget(sortButton);
    controlsLayout->addWidget(statsLabel);
    controlsLayout->addStretch();
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addLayout(controlsLayout);
    
    mainLayout->addWidget(headerWidget);

    // Scroll area with media grid
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background-color: #1a1a1a; border: none; }");
    
    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::onScrollChanged);
    
    mediaGrid = new MediaGrid();
    connect(mediaGrid, &MediaGrid::imageClicked, this, [this](const QString &filePath, const QStringList &imagePaths) {
        Q_UNUSED(imagePaths);
        // Collect all media paths (images and videos) in order
        QStringList allMediaPaths;
        for (const QFileInfo &fi : allFiles) {
            allMediaPaths.append(fi.absoluteFilePath());
        }
        
        ImageViewer *viewer = new ImageViewer(filePath, allMediaPaths);
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->show();
    });
    
    scrollArea->setWidget(mediaGrid);
    mainLayout->addWidget(scrollArea);

    setCentralWidget(centralWidget);
}

void MainWindow::openFolder()
{
    // Save current scroll position
    lastScrollPosition = scrollArea->verticalScrollBar()->value();
    
    QString directory = QFileDialog::getExistingDirectory(
        this,
        "选择包含媒体文件的文件夹",
        lastDirectory.isEmpty() ? QDir::homePath() : lastDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!directory.isEmpty()) {
        lastDirectory = directory;
        loadMediaFiles(directory, false);  // Don't restore scroll for new folder
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    int currentScroll = scrollArea->verticalScrollBar()->value();
    QSettings settings("MediaViewer", "MediaViewer");
    settings.setValue("lastDirectory", lastDirectory);
    settings.setValue("sortAscending", sortAscending);
    settings.setValue("scrollPosition", currentScroll);
    settings.sync();  // Force write to disk
    qDebug() << "Saving settings - Directory:" << lastDirectory 
             << "ScrollPos:" << currentScroll 
             << "SortAsc:" << sortAscending;
    QMainWindow::closeEvent(event);
}

void MainWindow::loadMediaFiles(const QString &directory, bool restoreScroll)
{
    mediaGrid->clear();
    allFiles.clear();
    currentBatch = 0;
    imageCount = 0;
    videoCount = 0;

    QDir dir(directory);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &fileInfo : fileList) {
        QString ext = fileInfo.suffix().toLower();
        if (imageExtensions.contains(ext)) {
            allFiles.append(fileInfo);
            imageCount++;
        } else if (videoExtensions.contains(ext)) {
            allFiles.append(fileInfo);
            videoCount++;
        }
    }
    
    // Sort by modified time based on sortAscending flag
    std::sort(allFiles.begin(), allFiles.end(), [this](const QFileInfo &a, const QFileInfo &b) {
        if (sortAscending) {
            return a.lastModified() < b.lastModified();
        } else {
            return a.lastModified() > b.lastModified();
        }
    });

    updateStats();
    loadNextBatch();
    
    // Restore scroll position if requested
    if (restoreScroll && lastScrollPosition > 0) {
        qDebug() << "Restoring scroll position:" << lastScrollPosition;
        // Use multiple timers to ensure layout is complete
        QTimer::singleShot(150, this, [this]() {
            int maxScroll = scrollArea->verticalScrollBar()->maximum();
            int targetPos = qMin(lastScrollPosition, maxScroll);
            scrollArea->verticalScrollBar()->setValue(targetPos);
            qDebug() << "Scroll restored to:" << targetPos << "max:" << maxScroll;
        });
    }
}

void MainWindow::loadNextBatch()
{
    int start = currentBatch * batchSize;
    int end = qMin(start + batchSize, allFiles.size());

    if (start >= allFiles.size()) {
        return;
    }

    for (int i = start; i < end; ++i) {
        mediaGrid->addMediaItem(allFiles[i]);
    }

    currentBatch++;
    // Update visible items after adding new batch
    QRect visibleRect = QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0, 0)),
                              scrollArea->viewport()->size());
    mediaGrid->updateVisibleItems(visibleRect);
}

void MainWindow::updateStats()
{
    statsLabel->setText(QString("共 %1 个文件 | %2 张图片 | %3 个视频")
                           .arg(allFiles.size())
                           .arg(imageCount)
                           .arg(videoCount));
}

void MainWindow::onScrollChanged(int value)
{
    Q_UNUSED(value);
    QRect visibleRect = QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0, 0)),
                              scrollArea->viewport()->size());
    mediaGrid->updateVisibleItems(visibleRect);
    loadTimer->start();
}

void MainWindow::checkLoadMore()
{
    QScrollBar *vScrollBar = scrollArea->verticalScrollBar();
    int scrollPos = vScrollBar->value();
    int maxScroll = vScrollBar->maximum();
    
    // Load more when near bottom (within 500px)
    if (maxScroll - scrollPos < 500 && currentBatch * batchSize < allFiles.size()) {
        loadNextBatch();
    }
}

void MainWindow::toggleSortOrder()
{
    sortAscending = !sortAscending;
    // Save current scroll position, then re-sort and reload
    lastScrollPosition = scrollArea->verticalScrollBar()->value();
    loadMediaFiles(lastDirectory, false);  // Don't restore scroll for sort toggle
    // Reset scroll to top when sort changes
    scrollArea->verticalScrollBar()->setValue(0);
}

// MediaGrid implementation
MediaGrid::MediaGrid(QWidget *parent)
    : QWidget(parent)
    , columns(5)
    , itemSize(250)
    , spacing(20)
{
    setStyleSheet("background-color: #1a1a1a;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MediaGrid::addMediaItem(const QFileInfo &fileInfo)
{
    MediaItem *item = new MediaItem(fileInfo, this);
    item->setFixedSize(itemSize, itemSize);
    
    connect(item, &MediaItem::clicked, this, [this, fileInfo]() {
        // Collect all image paths for navigation
        QStringList imagePaths;
        QString ext = fileInfo.suffix().toLower();
        QStringList imageExts;
        imageExts << "jpg" << "jpeg" << "png" << "gif" << "bmp" << "webp";
        
        if (imageExts.contains(ext)) {
            QDir dir = fileInfo.dir();
            QFileInfoList fileList = dir.entryInfoList(QDir::Files, QDir::Name);
            
            for (const QFileInfo &fi : fileList) {
                if (imageExts.contains(fi.suffix().toLower())) {
                    imagePaths.append(fi.absoluteFilePath());
                }
            }
            
            emit imageClicked(fileInfo.absoluteFilePath(), imagePaths);
        }
    });
    
    items.append(item);
    relayout();
}

void MediaGrid::clear()
{
    qDeleteAll(items);
    items.clear();
    setMinimumHeight(0);
}

void MediaGrid::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MediaGrid::relayout()
{
    int availableWidth = width() - 40; // padding
    columns = qMax(1, availableWidth / (itemSize + spacing));
    
    int row = 0, col = 0;
    for (QWidget *item : items) {
        int x = 20 + col * (itemSize + spacing);
        int y = 20 + row * (itemSize + spacing);
        item->move(x, y);
        item->show();
        
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
    
    int rows = (items.size() + columns - 1) / columns;
    int totalHeight = 20 + rows * (itemSize + spacing) + 20;
    setMinimumHeight(totalHeight);
}

void MediaGrid::updateVisibleItems(const QRect &visibleRect)
{
    for (QWidget *widget : items) {
        MediaItem *item = static_cast<MediaItem*>(widget);
        if (!item) {
            continue;
        }
        const QRect itemRect = item->geometry();
        const bool isVisible = itemRect.intersects(visibleRect);
        item->setActive(isVisible);
    }
}

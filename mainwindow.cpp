#include "mainwindow.h"
#include "mediaitem.h"
#include "zipmediaitem.h"
#include "zipimageviewer.h"
#include <QDir>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <algorithm>
#include <QFileIconProvider>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentBatch(0)
    , batchSize(30)
    , imageCount(0)
    , videoCount(0)
    , historyIndex(-1)
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

    QSettings settings("MediaViewer", "MediaViewer");
    currentPath = settings.value("currentPath", "").toString();
    sortAscending = settings.value("sortAscending", false).toBool();
    lastScrollPosition = settings.value("scrollPosition", 0).toInt();

    if (!currentPath.isEmpty()) {
        if (isZipFile(currentPath)) {
            loadZipFile(currentPath);
        } else {
            populateTree(currentPath);
            loadMediaFiles(currentPath, true);
        }
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("媒体文件查看器");
    resize(1400, 900);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ========== 顶部工具栏 ==========
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: #2b2b2b;");
    headerWidget->setFixedHeight(50);

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(10, 5, 10, 5);

    backButton = new QPushButton("◀");
    backButton->setFixedSize(30, 30);
    backButton->setStyleSheet(
        "QPushButton { background-color: #4a4a4a; color: white; border: none; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background-color: #5a5a5a; }"
        "QPushButton:disabled { background-color: #3a3a3a; color: #666; }"
    );
    backButton->setEnabled(false);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::goBack);

    upButton = new QPushButton("▲");
    upButton->setFixedSize(30, 30);
    upButton->setStyleSheet(backButton->styleSheet());
    upButton->setEnabled(false);
    connect(upButton, &QPushButton::clicked, this, &MainWindow::goUp);

    pathEdit = new QLineEdit();
    pathEdit->setStyleSheet(
        "QLineEdit { background-color: #3a3a3a; color: white; border: 1px solid #4a4a4a; border-radius: 4px; padding: 5px 10px; font-size: 12px; }"
        "QLineEdit:focus { border-color: #2563eb; }"
    );
    pathEdit->setPlaceholderText("输入路径...");
    connect(pathEdit, &QLineEdit::returnPressed, this, &MainWindow::onPathEditReturnPressed);

    QPushButton *openFolderBtn = new QPushButton("📁 文件夹");
    openFolderBtn->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    connect(openFolderBtn, &QPushButton::clicked, this, &MainWindow::openFolder);

    QPushButton *openZipBtn = new QPushButton("📦 ZIP");
    openZipBtn->setStyleSheet(
        "QPushButton { background-color: #059669; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #047857; }"
    );
    connect(openZipBtn, &QPushButton::clicked, this, &MainWindow::openZipFile);

    sortButton = new QPushButton("排序: 新到旧");
    sortButton->setStyleSheet(
        "QPushButton { background-color: #6b7280; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #4b5563; }"
    );
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::toggleSortOrder);

    statsLabel = new QLabel("未加载文件");
    statsLabel->setStyleSheet("color: #999; font-size: 12px; padding: 0 10px;");

    headerLayout->addWidget(backButton);
    headerLayout->addWidget(upButton);
    headerLayout->addWidget(pathEdit, 1);
    headerLayout->addWidget(openFolderBtn);
    headerLayout->addWidget(openZipBtn);
    headerLayout->addWidget(sortButton);
    headerLayout->addWidget(statsLabel);

    mainLayout->addWidget(headerWidget);

    // ========== 主内容区域 ==========
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: #3a3a3a; }");

    setupFileTree();
    setupMediaArea();

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(scrollArea);
    mainSplitter->setSizes(QList<int>() << 220 << 1180);

    mainLayout->addWidget(mainSplitter);
}

void MainWindow::setupFileTree()
{
    leftPanel = new QWidget();
    leftPanel->setStyleSheet("background-color: #252526;");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    QLabel *treeTitle = new QLabel("文件浏览器");
    treeTitle->setStyleSheet(
        "color: #cccccc; font-size: 12px; font-weight: bold; padding: 8px 12px; "
        "background-color: #333333; border-bottom: 1px solid #3a3a3a;"
    );
    leftLayout->addWidget(treeTitle);

    fileTree = new QTreeWidget();
    fileTree->setStyleSheet(
        "QTreeWidget { background-color: #252526; color: #cccccc; border: none; font-size: 12px; }"
        "QTreeWidget::item { padding: 4px; border: none; }"
        "QTreeWidget::item:selected { background-color: #094771; }"
        "QTreeWidget::item:hover { background-color: #2a2d2e; }"
        "QHeaderView::section { background-color: #333333; color: #cccccc; border: none; padding: 5px; }"
    );
    fileTree->setHeaderLabels(QStringList() << "名称" << "类型");
    fileTree->header()->setStretchLastSection(true);
    fileTree->setColumnWidth(0, 160);
    fileTree->setIndentation(15);
    fileTree->setAnimated(true);

    connect(fileTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onTreeItemDoubleClicked);

    leftLayout->addWidget(fileTree);
}

void MainWindow::setupMediaArea()
{
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background-color: #1a1a1a; border: none; }");

    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollChanged);

    mediaGrid = new MediaGrid();
    connect(mediaGrid, &MediaGrid::imageClicked, this, [this](const QString &filePath, const QStringList &) {
        QStringList allMediaPaths;
        for (const QFileInfo &fi : allFiles) allMediaPaths.append(fi.absoluteFilePath());
        ImageViewer *viewer = new ImageViewer(filePath, allMediaPaths);
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->show();
    });

    connect(mediaGrid, &MediaGrid::zipMediaClicked, this, [this](const QString &filePath, const QList<ZipReader::ZipEntry> &, QSharedPointer<ZipReader> zipReader) {
        ZipImageViewer *viewer = new ZipImageViewer(filePath, zipEntries, zipReader);
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->show();
    });

    connect(mediaGrid, &MediaGrid::zipFileClicked, this, [this](const QString &zipPath) {
        loadZipFile(zipPath);
    });

    scrollArea->setWidget(mediaGrid);
}

void MainWindow::populateTree(const QString &path)
{
    fileTree->clear();
    QDir dir(path);
    if (!dir.exists()) return;

    QFileIconProvider iconProvider;

    QTreeWidgetItem *parentItem = new QTreeWidgetItem();
    parentItem->setText(0, "..");
    parentItem->setText(1, "上级目录");
    parentItem->setData(0, Qt::UserRole, dir.absolutePath());
    parentItem->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
    fileTree->addTopLevelItem(parentItem);

    QFileInfoList folders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &folder : folders) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, folder.fileName());
        item->setText(1, "文件夹");
        item->setData(0, Qt::UserRole, folder.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, "folder");
        item->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
        fileTree->addTopLevelItem(item);
    }

    QFileInfoList zipFiles = dir.entryInfoList(QStringList() << "*.zip", QDir::Files, QDir::Name);
    for (const QFileInfo &zipFile : zipFiles) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, zipFile.fileName());
        item->setText(1, "ZIP");
        item->setData(0, Qt::UserRole, zipFile.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, "zip");
        item->setIcon(0, iconProvider.icon(QFileIconProvider::File));
        fileTree->addTopLevelItem(item);
    }

    QFileInfoList allFilesList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    int imgCount = 0, vidCount = 0;
    for (const QFileInfo &file : allFilesList) {
        QString ext = file.suffix().toLower();
        if (imageExtensions.contains(ext)) imgCount++;
        else if (videoExtensions.contains(ext)) vidCount++;
    }

    if (imgCount > 0 || vidCount > 0) {
        QTreeWidgetItem *mediaItem = new QTreeWidgetItem();
        mediaItem->setText(0, QString("媒体 (%1 图, %2 视)").arg(imgCount).arg(vidCount));
        mediaItem->setText(1, "");
        mediaItem->setData(0, Qt::UserRole, path);
        mediaItem->setData(0, Qt::UserRole + 1, "media");
        mediaItem->setFlags(mediaItem->flags() & ~Qt::ItemIsSelectable);
        mediaItem->setForeground(0, QColor("#888888"));
        fileTree->addTopLevelItem(mediaItem);
    }

    pathEdit->setText(path);
    updateNavigationButtons(path);
}

void MainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item, int)
{
    QString path = item->data(0, Qt::UserRole).toString();
    QString type = item->data(0, Qt::UserRole + 1).toString();

    if (type == "zip") {
        loadZipFile(path);
    } else if (type == "folder" || item->text(0) == "..") {
        QDir dir(path);
        if (dir.exists()) {
            QString targetPath = item->text(0) == ".." ? dir.absolutePath() : path;
            addToHistory(targetPath);
            currentPath = targetPath;
            populateTree(targetPath);
            loadMediaFiles(targetPath, false);
        }
    }
}

void MainWindow::onPathEditReturnPressed()
{
    QString path = pathEdit->text().trimmed();
    if (path.isEmpty()) return;

    if (isZipFile(path)) {
        loadZipFile(path);
    } else {
        QDir dir(path);
        if (dir.exists()) {
            addToHistory(path);
            currentPath = path;
            populateTree(path);
            loadMediaFiles(path, false);
        } else {
            QMessageBox::warning(this, "错误", "路径不存在: " + path);
        }
    }
}

void MainWindow::goBack()
{
    if (historyIndex > 0) {
        historyIndex--;
        QString path = navigationHistory[historyIndex];
        currentPath = path;
        if (isZipFile(path)) {
            loadZipFile(path);
        } else {
            populateTree(path);
            loadMediaFiles(path, false);
        }
        updateNavigationButtons(path);
    }
}

void MainWindow::goUp()
{
    if (currentPath.isEmpty()) return;
    QDir dir(currentPath);
    if (dir.cdUp()) {
        QString parentPath = dir.absolutePath();
        addToHistory(parentPath);
        currentPath = parentPath;
        populateTree(parentPath);
        loadMediaFiles(parentPath, false);
    }
}

void MainWindow::updateNavigationButtons(const QString &path)
{
    backButton->setEnabled(historyIndex > 0);
    upButton->setEnabled(!path.isEmpty() && QDir(path).cdUp());
}

void MainWindow::addToHistory(const QString &path)
{
    if (historyIndex < navigationHistory.size() - 1) {
        navigationHistory = navigationHistory.mid(0, historyIndex + 1);
    }
    navigationHistory.append(path);
    historyIndex = navigationHistory.size() - 1;
    updateNavigationButtons(path);
}

bool MainWindow::isZipFile(const QString &path) const
{
    return path.endsWith(".zip", Qt::CaseInsensitive);
}

void MainWindow::openFolder()
{
    QString directory = QFileDialog::getExistingDirectory(
        this, "选择文件夹",
        currentPath.isEmpty() ? QDir::homePath() : currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!directory.isEmpty()) {
        addToHistory(directory);
        currentPath = directory;
        populateTree(directory);
        loadMediaFiles(directory, false);
    }
}

void MainWindow::openZipFile()
{
    QString zipPath = QFileDialog::getOpenFileName(
        this, "选择ZIP压缩包",
        currentPath.isEmpty() ? QDir::homePath() : currentPath,
        "ZIP文件 (*.zip);;所有文件 (*.*)"
    );
    if (!zipPath.isEmpty()) {
        addToHistory(zipPath);
        currentPath = zipPath;
        loadZipFile(zipPath);
    }
}

void MainWindow::loadZipFile(const QString &zipPath)
{
    currentZipReader.reset();
    zipEntries.clear();
    mediaGrid->clear();
    mediaGrid->setZipReader(nullptr);
    allFiles.clear();
    currentBatch = 0;
    imageCount = 0;
    videoCount = 0;

    fileTree->clear();
    QFileIconProvider iconProvider;

    QFileInfo zipInfo(zipPath);
    QTreeWidgetItem *parentItem = new QTreeWidgetItem();
    parentItem->setText(0, "..");
    parentItem->setText(1, "上级目录");
    parentItem->setData(0, Qt::UserRole, zipInfo.absolutePath());
    parentItem->setData(0, Qt::UserRole + 1, "folder");
    parentItem->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
    fileTree->addTopLevelItem(parentItem);

    currentZipReader = QSharedPointer<ZipReader>::create(zipPath);

    QString errorString;
    if (!currentZipReader->open(&errorString)) {
        if (currentZipReader->needsPassword()) {
            bool ok;
            QString password = QInputDialog::getText(this, "ZIP密码", "请输入密码:", QLineEdit::Password, "", &ok);
            if (ok && !password.isEmpty()) {
                if (!currentZipReader->openWithPassword(password, &errorString)) {
                    QMessageBox::warning(this, "错误", QString("无法打开ZIP:\n%1").arg(errorString));
                    currentZipReader.reset();
                    statsLabel->setText("未加载文件");
                    return;
                }
            } else {
                currentZipReader.reset();
                statsLabel->setText("未加载文件");
                return;
            }
        } else {
            QMessageBox::warning(this, "错误", QString("无法打开ZIP:\n%1").arg(errorString));
            currentZipReader.reset();
            statsLabel->setText("未加载文件");
            return;
        }
    }

    zipEntries = currentZipReader->getMediaEntries();
    mediaGrid->setZipEntries(zipEntries);
    mediaGrid->setZipReader(currentZipReader);

    for (const ZipReader::ZipEntry &entry : zipEntries) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, entry.fileName);
        item->setText(1, ZipReader::isImageFile(entry.fileName) ? "图片" : "视频");
        item->setData(0, Qt::UserRole, entry.filePath);
        item->setData(0, Qt::UserRole + 1, "zip_entry");
        fileTree->addTopLevelItem(item);

        if (ZipReader::isImageFile(entry.fileName)) imageCount++;
        else if (ZipReader::isVideoFile(entry.fileName)) videoCount++;
    }

    pathEdit->setText(zipPath + " (ZIP)");
    updateStats();
    loadNextBatch();
    setWindowTitle(QString("媒体文件查看器 - %1").arg(zipInfo.fileName()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("MediaViewer", "MediaViewer");
    settings.setValue("currentPath", currentPath);
    settings.setValue("sortAscending", sortAscending);
    settings.setValue("scrollPosition", scrollArea->verticalScrollBar()->value());
    QMainWindow::closeEvent(event);
}

void MainWindow::clearCurrentView()
{
    mediaGrid->clear();
    mediaGrid->setZipReader(nullptr);
    allFiles.clear();
    zipEntries.clear();
    currentZipReader.reset();
    currentBatch = 0;
    imageCount = 0;
    videoCount = 0;
}

void MainWindow::loadMediaFiles(const QString &directory, bool restoreScroll)
{
    clearCurrentView();
    QDir dir(directory);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &fileInfo : fileList) {
        QString ext = fileInfo.suffix().toLower();
        if (imageExtensions.contains(ext)) { allFiles.append(fileInfo); imageCount++; }
        else if (videoExtensions.contains(ext)) { allFiles.append(fileInfo); videoCount++; }
        else if (ext == "zip") { mediaGrid->addZipFileItem(fileInfo.absoluteFilePath()); }
    }

    std::sort(allFiles.begin(), allFiles.end(), [this](const QFileInfo &a, const QFileInfo &b) {
        return sortAscending ? a.lastModified() < b.lastModified() : a.lastModified() > b.lastModified();
    });

    updateStats();
    loadNextBatch();

    if (restoreScroll && lastScrollPosition > 0) {
        QTimer::singleShot(150, this, [this]() {
            scrollArea->verticalScrollBar()->setValue(qMin(lastScrollPosition, scrollArea->verticalScrollBar()->maximum()));
        });
    }
}

void MainWindow::loadNextBatch()
{
    int start = currentBatch * batchSize;
    int end = qMin(start + batchSize, allFiles.size());

    if (!zipEntries.isEmpty() && currentZipReader) {
        end = qMin(start + batchSize, zipEntries.size());
        if (start >= zipEntries.size()) return;
        for (int i = start; i < end; ++i) mediaGrid->addZipMediaItem(zipEntries[i], currentZipReader);
        currentBatch++;
        mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0, 0)), scrollArea->viewport()->size()));
        return;
    }

    if (start >= allFiles.size()) return;
    for (int i = start; i < end; ++i) mediaGrid->addMediaItem(allFiles[i]);
    currentBatch++;
    mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0, 0)), scrollArea->viewport()->size()));
}

void MainWindow::updateStats()
{
    if (!zipEntries.isEmpty()) {
        statsLabel->setText(QString("ZIP: %1 | %2 图 | %3 视").arg(zipEntries.size()).arg(imageCount).arg(videoCount));
    } else {
        statsLabel->setText(QString("%1 | %2 图 | %3 视").arg(allFiles.size()).arg(imageCount).arg(videoCount));
    }
}

void MainWindow::onScrollChanged(int)
{
    mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0, 0)), scrollArea->viewport()->size()));
    loadTimer->start();
}

void MainWindow::checkLoadMore()
{
    int totalItems = zipEntries.isEmpty() ? allFiles.size() : zipEntries.size();
    if (scrollArea->verticalScrollBar()->maximum() - scrollArea->verticalScrollBar()->value() < 500 && currentBatch * batchSize < totalItems) {
        loadNextBatch();
    }
}

void MainWindow::toggleSortOrder()
{
    sortAscending = !sortAscending;
    lastScrollPosition = scrollArea->verticalScrollBar()->value();
    sortButton->setText(sortAscending ? "排序: 旧到新" : "排序: 新到旧");

    if (!zipEntries.isEmpty()) {
        std::sort(zipEntries.begin(), zipEntries.end(), [](const ZipReader::ZipEntry &a, const ZipReader::ZipEntry &b) {
            return a.filePath < b.filePath;
        });
        mediaGrid->clear();
        mediaGrid->setZipEntries(zipEntries);
        currentBatch = 0;
        loadNextBatch();
    } else {
        loadMediaFiles(currentPath, false);
    }
    scrollArea->verticalScrollBar()->setValue(0);
}

// MediaGrid
MediaGrid::MediaGrid(QWidget *parent) : QWidget(parent), columns(5), itemSize(250), spacing(20)
{
    setStyleSheet("background-color: #1a1a1a;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MediaGrid::addMediaItem(const QFileInfo &fileInfo)
{
    MediaItem *item = new MediaItem(fileInfo, this);
    item->setFixedSize(itemSize, itemSize);
    connect(item, &MediaItem::clicked, this, [this, fileInfo]() {
        emit imageClicked(fileInfo.absoluteFilePath(), QStringList());
    });
    items.append(item);
    relayout();
}

void MediaGrid::addZipMediaItem(const ZipReader::ZipEntry &entry, QSharedPointer<ZipReader> reader)
{
    ZipMediaItem *item = new ZipMediaItem(entry, reader, this);
    item->setFixedSize(itemSize, itemSize);
    connect(item, &ZipMediaItem::clicked, this, [this, entry, reader]() {
        emit zipMediaClicked(entry.filePath, zipEntries, reader);
    });
    items.append(item);
    relayout();
}

void MediaGrid::addZipFileItem(const QString &zipPath)
{
    QWidget *item = new QWidget(this);
    item->setFixedSize(itemSize, itemSize);
    item->setStyleSheet("background-color: #2a2a2a; border-radius: 12px;");
    item->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *layout = new QVBoxLayout(item);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *iconLabel = new QLabel("📦", item);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px; background: transparent;");
    layout->addWidget(iconLabel);

    QLabel *nameLabel = new QLabel(QFileInfo(zipPath).fileName(), item);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("color: white; font-size: 11px; padding: 5px; background: transparent;");
    nameLabel->setWordWrap(true);
    layout->addWidget(nameLabel);

    item->setProperty("zipPath", zipPath);
    items.append(item);
    relayout();
}

void MediaGrid::clear()
{
    qDeleteAll(items);
    items.clear();
    zipEntries.clear();
    zipReader.clear();
    setMinimumHeight(0);
}

void MediaGrid::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MediaGrid::relayout()
{
    int availableWidth = width() - 40;
    columns = qMax(1, availableWidth / (itemSize + spacing));

    int row = 0, col = 0;
    for (QWidget *item : items) {
        item->move(20 + col * (itemSize + spacing), 20 + row * (itemSize + spacing));
        item->show();
        if (++col >= columns) { col = 0; row++; }
    }

    setMinimumHeight(20 + ((items.size() + columns - 1) / columns) * (itemSize + spacing) + 20);
}

void MediaGrid::updateVisibleItems(const QRect &visibleRect)
{
    for (QWidget *widget : items) {
        if (auto *item = qobject_cast<MediaItem*>(widget))
            item->setActive(item->geometry().intersects(visibleRect));
        else if (auto *zipItem = qobject_cast<ZipMediaItem*>(widget))
            zipItem->setActive(zipItem->geometry().intersects(visibleRect));
    }
}

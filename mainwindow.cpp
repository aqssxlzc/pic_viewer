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
#include <QMouseEvent>
#include <algorithm>
#include <QFileIconProvider>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentBatch(0), batchSize(30), imageCount(0), videoCount(0),
      historyIndex(-1), sortAscending(false), lastScrollPosition(0)
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
        if (isZipFile(currentPath)) loadZipFile(currentPath);
        else { populateTree(currentPath); loadMediaFiles(currentPath, true); }
    }
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    setWindowTitle("媒体文件查看器");
    resize(1400, 900);
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *header = new QWidget();
    header->setStyleSheet("background-color: #2b2b2b;");
    header->setFixedHeight(50);
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(10, 5, 10, 5);

    backButton = new QPushButton("◀");
    backButton->setFixedSize(30, 30);
    backButton->setStyleSheet("QPushButton{background:#4a4a4a;color:white;border:none;border-radius:4px;}QPushButton:hover{background:#5a5a5a;}QPushButton:disabled{background:#3a3a3a;color:#666;}");
    backButton->setEnabled(false);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::goBack);

    upButton = new QPushButton("▲");
    upButton->setFixedSize(30, 30);
    upButton->setStyleSheet(backButton->styleSheet());
    upButton->setEnabled(false);
    connect(upButton, &QPushButton::clicked, this, &MainWindow::goUp);

    pathEdit = new QLineEdit();
    pathEdit->setStyleSheet("QLineEdit{background:#3a3a3a;color:white;border:1px solid #4a4a4a;border-radius:4px;padding:5px 10px;font-size:12px;}QLineEdit:focus{border-color:#2563eb;}");
    pathEdit->setPlaceholderText("输入路径...");
    connect(pathEdit, &QLineEdit::returnPressed, this, &MainWindow::onPathEditReturnPressed);

    QPushButton *btnFolder = new QPushButton("📁 文件夹");
    btnFolder->setStyleSheet("QPushButton{background:#2563eb;color:white;border:none;padding:6px 12px;border-radius:4px;}QPushButton:hover{background:#1d4ed8;}");
    connect(btnFolder, &QPushButton::clicked, this, &MainWindow::openFolder);

    QPushButton *btnZip = new QPushButton("📦 ZIP");
    btnZip->setStyleSheet("QPushButton{background:#059669;color:white;border:none;padding:6px 12px;border-radius:4px;}QPushButton:hover{background:#047857;}");
    connect(btnZip, &QPushButton::clicked, this, &MainWindow::openZipFile);

    sortButton = new QPushButton("排序: 新到旧");
    sortButton->setStyleSheet("QPushButton{background:#6b7280;color:white;border:none;padding:6px 12px;border-radius:4px;}QPushButton:hover{background:#4b5563;}");
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::toggleSortOrder);

    statsLabel = new QLabel("未加载文件");
    statsLabel->setStyleSheet("color:#999;font-size:12px;padding:0 10px;");

    hl->addWidget(backButton);
    hl->addWidget(upButton);
    hl->addWidget(pathEdit, 1);
    hl->addWidget(btnFolder);
    hl->addWidget(btnZip);
    hl->addWidget(sortButton);
    hl->addWidget(statsLabel);
    mainLayout->addWidget(header);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setStyleSheet("QSplitter::handle{background:#3a3a3a;}");

    setupFileTree();
    setupMediaArea();
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(scrollArea);
    mainSplitter->setSizes(QList<int>() << 220 << 1180);
    mainLayout->addWidget(mainSplitter);
}

void MainWindow::setupFileTree() {
    leftPanel = new QWidget();
    leftPanel->setStyleSheet("background-color:#252526;");
    QVBoxLayout *ll = new QVBoxLayout(leftPanel);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(0);

    QLabel *tt = new QLabel("文件浏览器");
    tt->setStyleSheet("color:#ccc;font-size:12px;font-weight:bold;padding:8px 12px;background:#333;border-bottom:1px solid #3a3a3a;");
    ll->addWidget(tt);

    fileTree = new QTreeWidget();
    fileTree->setStyleSheet("QTreeWidget{background:#252526;color:#ccc;border:none;font-size:12px;}QTreeWidget::item{padding:4px;}QTreeWidget::item:selected{background:#094771;}QTreeWidget::item:hover{background:#2a2d2e;}QHeaderView::section{background:#333;color:#ccc;border:none;padding:5px;}");
    fileTree->setHeaderLabels(QStringList() << "名称" << "类型");
    fileTree->header()->setStretchLastSection(true);
    fileTree->setColumnWidth(0, 160);
    fileTree->setIndentation(15);
    connect(fileTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onTreeItemDoubleClicked);
    ll->addWidget(fileTree);
}

void MainWindow::setupMediaArea() {
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea{background:#1a1a1a;border:none;}");
    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollChanged);

    mediaGrid = new MediaGrid();
    connect(mediaGrid, &MediaGrid::imageClicked, this, [this](const QString &fp, const QStringList &) {
        QStringList paths;
        for (auto &f : allFiles) paths << f.absoluteFilePath();
        auto *v = new ImageViewer(fp, paths);
        v->setAttribute(Qt::WA_DeleteOnClose);
        v->show();
    });
    connect(mediaGrid, &MediaGrid::zipMediaClicked, this, [this](const QString &fp, const QList<ZipReader::ZipEntry> &, QSharedPointer<ZipReader> zr) {
        auto *v = new ZipImageViewer(fp, zipEntries, zr);
        v->setAttribute(Qt::WA_DeleteOnClose);
        v->show();
    });
    connect(mediaGrid, &MediaGrid::zipFileClicked, this, [this](const QString &path) {
        QMetaObject::invokeMethod(this, [this, path]() { loadZipFile(path); }, Qt::QueuedConnection);
    });
    scrollArea->setWidget(mediaGrid);
}

void MainWindow::populateTree(const QString &path) {
    fileTree->clear();
    QDir dir(path);
    if (!dir.exists()) return;
    QFileIconProvider ip;

    auto *pi = new QTreeWidgetItem();
    pi->setText(0, "..");
    pi->setText(1, "上级");
    pi->setData(0, Qt::UserRole, dir.absolutePath());
    pi->setData(0, Qt::UserRole + 1, "folder");
    pi->setIcon(0, ip.icon(QFileIconProvider::Folder));
    fileTree->addTopLevelItem(pi);

    for (auto &f : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        auto *i = new QTreeWidgetItem();
        i->setText(0, f.fileName());
        i->setText(1, "文件夹");
        i->setData(0, Qt::UserRole, f.absoluteFilePath());
        i->setData(0, Qt::UserRole + 1, "folder");
        i->setIcon(0, ip.icon(QFileIconProvider::Folder));
        fileTree->addTopLevelItem(i);
    }
    for (auto &f : dir.entryInfoList(QStringList() << "*.zip", QDir::Files, QDir::Name)) {
        auto *i = new QTreeWidgetItem();
        i->setText(0, f.fileName());
        i->setText(1, "ZIP");
        i->setData(0, Qt::UserRole, f.absoluteFilePath());
        i->setData(0, Qt::UserRole + 1, "zip");
        i->setIcon(0, ip.icon(QFileIconProvider::File));
        fileTree->addTopLevelItem(i);
    }

    int ic = 0, vc = 0;
    for (auto &f : dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
        QString e = f.suffix().toLower();
        if (imageExtensions.contains(e)) ic++;
        else if (videoExtensions.contains(e)) vc++;
    }
    if (ic > 0 || vc > 0) {
        auto *mi = new QTreeWidgetItem();
        mi->setText(0, QString("媒体 (%1图,%2视)").arg(ic).arg(vc));
        mi->setData(0, Qt::UserRole, path);
        mi->setData(0, Qt::UserRole + 1, "media");
        mi->setFlags(mi->flags() & ~Qt::ItemIsSelectable);
        mi->setForeground(0, QColor("#888"));
        fileTree->addTopLevelItem(mi);
    }
    pathEdit->setText(path);
    updateNavigationButtons(path);
}

void MainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item, int) {
    QString path = item->data(0, Qt::UserRole).toString();
    QString type = item->data(0, Qt::UserRole + 1).toString();
    if (type == "zip") { loadZipFile(path); }
    else if (type == "folder" || item->text(0) == "..") {
        QDir d(path);
        if (d.exists()) {
            QString tp = item->text(0) == ".." ? d.absolutePath() : path;
            addToHistory(tp);
            currentPath = tp;
            populateTree(tp);
            loadMediaFiles(tp, false);
        }
    }
}

void MainWindow::onPathEditReturnPressed() {
    QString p = pathEdit->text().trimmed();
    if (p.isEmpty()) return;
    if (isZipFile(p)) loadZipFile(p);
    else {
        QDir d(p);
        if (d.exists()) { addToHistory(p); currentPath = p; populateTree(p); loadMediaFiles(p, false); }
        else QMessageBox::warning(this, "错误", "路径不存在: " + p);
    }
}

void MainWindow::goBack() {
    if (historyIndex > 0) {
        historyIndex--;
        QString p = navigationHistory[historyIndex];
        currentPath = p;
        if (isZipFile(p)) loadZipFile(p);
        else { populateTree(p); loadMediaFiles(p, false); }
        updateNavigationButtons(p);
    }
}

void MainWindow::goUp() {
    if (currentPath.isEmpty()) return;
    QDir d(currentPath);
    if (d.cdUp()) {
        addToHistory(d.absolutePath());
        currentPath = d.absolutePath();
        populateTree(currentPath);
        loadMediaFiles(currentPath, false);
    }
}

void MainWindow::updateNavigationButtons(const QString &p) {
    backButton->setEnabled(historyIndex > 0);
    upButton->setEnabled(!p.isEmpty() && QDir(p).cdUp());
}

void MainWindow::addToHistory(const QString &p) {
    if (historyIndex < navigationHistory.size() - 1) navigationHistory = navigationHistory.mid(0, historyIndex + 1);
    navigationHistory << p;
    historyIndex = navigationHistory.size() - 1;
    updateNavigationButtons(p);
}

bool MainWindow::isZipFile(const QString &p) const { return p.endsWith(".zip", Qt::CaseInsensitive); }

void MainWindow::openFolder() {
    QString d = QFileDialog::getExistingDirectory(this, "选择文件夹", currentPath.isEmpty() ? QDir::homePath() : currentPath, QFileDialog::ShowDirsOnly);
    if (!d.isEmpty()) { addToHistory(d); currentPath = d; populateTree(d); loadMediaFiles(d, false); }
}

void MainWindow::openZipFile() {
    QString z = QFileDialog::getOpenFileName(this, "选择ZIP", currentPath.isEmpty() ? QDir::homePath() : currentPath, "ZIP (*.zip)");
    if (!z.isEmpty()) { addToHistory(z); currentPath = z; loadZipFile(z); }
}

void MainWindow::loadZipFile(const QString &zipPath) {
    // 先清理
    currentZipReader.reset();
    zipEntries.clear();
    allFiles.clear();
    currentBatch = imageCount = videoCount = 0;
    mediaGrid->clear();
    mediaGrid->setZipReader(nullptr);

    fileTree->clear();
    QFileIconProvider ip;
    QFileInfo zi(zipPath);
    auto *pi = new QTreeWidgetItem();
    pi->setText(0, "..");
    pi->setText(1, "上级");
    pi->setData(0, Qt::UserRole, zi.absolutePath());
    pi->setData(0, Qt::UserRole + 1, "folder");
    pi->setIcon(0, ip.icon(QFileIconProvider::Folder));
    fileTree->addTopLevelItem(pi);

    currentZipReader = QSharedPointer<ZipReader>::create(zipPath);
    QString err;
    if (!currentZipReader->open(&err)) {
        if (currentZipReader->needsPassword()) {
            bool ok;
            QString pw = QInputDialog::getText(this, "密码", "输入密码:", QLineEdit::Password, "", &ok);
            if (ok && !pw.isEmpty()) {
                if (!currentZipReader->openWithPassword(pw, &err)) {
                    QMessageBox::warning(this, "错误", "无法打开:\n" + err);
                    currentZipReader.reset();
                    statsLabel->setText("未加载");
                    return;
                }
            } else { currentZipReader.reset(); statsLabel->setText("未加载"); return; }
        } else { QMessageBox::warning(this, "错误", "无法打开:\n" + err); currentZipReader.reset(); statsLabel->setText("未加载"); return; }
    }
    
    zipEntries = currentZipReader->getMediaEntries();
    // 限制条目数量防止内存问题
    const int maxEntries = 10000;
    if (zipEntries.size() > maxEntries) {
        zipEntries = zipEntries.mid(0, maxEntries);
    }
    
    mediaGrid->setZipEntries(zipEntries);
    mediaGrid->setZipReader(currentZipReader);
    for (auto &e : zipEntries) {
        auto *i = new QTreeWidgetItem();
        i->setText(0, e.fileName);
        i->setText(1, ZipReader::isImageFile(e.fileName) ? "图" : "视");
        i->setData(0, Qt::UserRole, e.filePath);
        i->setData(0, Qt::UserRole + 1, "zip_entry");
        fileTree->addTopLevelItem(i);
        if (ZipReader::isImageFile(e.fileName)) imageCount++;
        else if (ZipReader::isVideoFile(e.fileName)) videoCount++;
    }
    pathEdit->setText(zipPath + " (ZIP)");
    updateStats();
    loadNextBatch();
    setWindowTitle("媒体查看器 - " + zi.fileName());
}

void MainWindow::closeEvent(QCloseEvent *e) {
    QSettings s("MediaViewer", "MediaViewer");
    s.setValue("currentPath", currentPath);
    s.setValue("sortAscending", sortAscending);
    s.setValue("scrollPosition", scrollArea->verticalScrollBar()->value());
    QMainWindow::closeEvent(e);
}

void MainWindow::clearCurrentView() {
    mediaGrid->clear();
    mediaGrid->setZipReader(nullptr);
    allFiles.clear();
    zipEntries.clear();
    currentZipReader.reset();
    currentBatch = imageCount = videoCount = 0;
}

void MainWindow::loadMediaFiles(const QString &dir, bool restore) {
    clearCurrentView();
    QDir d(dir);
    for (auto &f : d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
        QString e = f.suffix().toLower();
        if (imageExtensions.contains(e)) { allFiles << f; imageCount++; }
        else if (videoExtensions.contains(e)) { allFiles << f; videoCount++; }
        else if (e == "zip") mediaGrid->addZipFileItem(f.absoluteFilePath());
    }
    std::sort(allFiles.begin(), allFiles.end(), [this](auto &a, auto &b) { return sortAscending ? a.lastModified() < b.lastModified() : a.lastModified() > b.lastModified(); });
    updateStats();
    loadNextBatch();
    if (restore && lastScrollPosition > 0) QTimer::singleShot(150, [this] { scrollArea->verticalScrollBar()->setValue(qMin(lastScrollPosition, scrollArea->verticalScrollBar()->maximum())); });
}

void MainWindow::loadNextBatch() {
    int st = currentBatch * batchSize, end = qMin(st + batchSize, allFiles.size());
    if (!zipEntries.isEmpty() && currentZipReader) {
        end = qMin(st + batchSize, zipEntries.size());
        if (st >= zipEntries.size()) return;
        for (int i = st; i < end; ++i) mediaGrid->addZipMediaItem(zipEntries[i], currentZipReader);
        currentBatch++;
        mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0,0)), scrollArea->viewport()->size()));
        return;
    }
    if (st >= allFiles.size()) return;
    for (int i = st; i < end; ++i) mediaGrid->addMediaItem(allFiles[i]);
    currentBatch++;
    mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0,0)), scrollArea->viewport()->size()));
}

void MainWindow::updateStats() {
    statsLabel->setText(zipEntries.isEmpty() ? QString("%1|%2图|%3视").arg(allFiles.size()).arg(imageCount).arg(videoCount) : QString("ZIP:%1|%2图|%3视").arg(zipEntries.size()).arg(imageCount).arg(videoCount));
}

void MainWindow::onScrollChanged(int) { mediaGrid->updateVisibleItems(QRect(mediaGrid->mapFrom(scrollArea->viewport(), QPoint(0,0)), scrollArea->viewport()->size())); loadTimer->start(); }

void MainWindow::checkLoadMore() {
    int total = zipEntries.isEmpty() ? allFiles.size() : zipEntries.size();
    if (scrollArea->verticalScrollBar()->maximum() - scrollArea->verticalScrollBar()->value() < 500 && currentBatch * batchSize < total) loadNextBatch();
}

void MainWindow::toggleSortOrder() {
    sortAscending = !sortAscending;
    sortButton->setText(sortAscending ? "排序:旧→新" : "排序:新→旧");
    lastScrollPosition = scrollArea->verticalScrollBar()->value();
    if (!zipEntries.isEmpty()) {
        std::sort(zipEntries.begin(), zipEntries.end(), [](auto &a, auto &b) { return a.filePath < b.filePath; });
        clearCurrentView();
        mediaGrid->setZipEntries(zipEntries);
        mediaGrid->setZipReader(currentZipReader);
        loadNextBatch();
    } else loadMediaFiles(currentPath, false);
    scrollArea->verticalScrollBar()->setValue(0);
}

// MediaGrid
MediaGrid::MediaGrid(QWidget *p) : QWidget(p), columns(5), itemSize(250), spacing(20) { setStyleSheet("background:#1a1a1a;"); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }

void MediaGrid::addMediaItem(const QFileInfo &fi) {
    auto *item = new MediaItem(fi, this);
    item->setFixedSize(itemSize, itemSize);
    connect(item, &MediaItem::clicked, this, [this, fi] { emit imageClicked(fi.absoluteFilePath(), QStringList()); });
    items << item; relayout();
}

void MediaGrid::addZipMediaItem(const ZipReader::ZipEntry &e, QSharedPointer<ZipReader> r) {
    auto *item = new ZipMediaItem(e, r, this);
    item->setFixedSize(itemSize, itemSize);
    connect(item, &ZipMediaItem::clicked, this, [this, e, r] { emit zipMediaClicked(e.filePath, zipEntries, r); });
    items << item; relayout();
}

void MediaGrid::addZipFileItem(const QString &zp) {
    auto *item = new QWidget(this);
    item->setFixedSize(itemSize, itemSize);
    item->setStyleSheet("background:#2a2a2a;border-radius:12px;");
    item->setCursor(Qt::PointingHandCursor);
    auto *l = new QVBoxLayout(item);
    l->setAlignment(Qt::AlignCenter);
    auto *icon = new QLabel("📦", item);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("font-size:48px;background:transparent;");
    l->addWidget(icon);
    auto *name = new QLabel(QFileInfo(zp).fileName(), item);
    name->setAlignment(Qt::AlignCenter);
    name->setStyleSheet("color:white;font-size:11px;padding:5px;background:transparent;");
    name->setWordWrap(true);
    l->addWidget(name);
    item->setProperty("zipPath", zp);
    item->installEventFilter(this);
    zipItemPaths[item] = zp;
    items << item; relayout();
}

void MediaGrid::clear() { 
    // 先移除事件过滤器
    for (auto it = zipItemPaths.begin(); it != zipItemPaths.end(); ++it) {
        it.key()->removeEventFilter(this);
    }
    zipItemPaths.clear();
    qDeleteAll(items);
    items.clear();
    zipEntries.clear();
    zipReader.clear();
    setMinimumHeight(0);
}

void MediaGrid::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); relayout(); }

bool MediaGrid::eventFilter(QObject *w, QEvent *e) {
    if (e->type() == QEvent::MouseButtonDblClick) {
        QWidget *wid = qobject_cast<QWidget*>(w);
        if (wid && zipItemPaths.contains(wid)) {
            QString path = zipItemPaths[wid];
            emit zipFileClicked(path);
            return true;
        }
    }
    return QWidget::eventFilter(w, e);
}

void MediaGrid::relayout() {
    int aw = width() - 40;
    columns = qMax(1, aw / (itemSize + spacing));
    int r = 0, c = 0;
    for (auto *item : items) {
        item->move(20 + c * (itemSize + spacing), 20 + r * (itemSize + spacing));
        item->show();
        if (++c >= columns) { c = 0; r++; }
    }
    setMinimumHeight(20 + ((items.size() + columns - 1) / columns) * (itemSize + spacing) + 20);
}

void MediaGrid::updateVisibleItems(const QRect &vr) {
    for (auto *w : items) {
        if (auto *i = qobject_cast<MediaItem*>(w)) i->setActive(i->geometry().intersects(vr));
        else if (auto *z = qobject_cast<ZipMediaItem*>(w)) z->setActive(z->geometry().intersects(vr));
    }
}

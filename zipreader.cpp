#include "zipreader.h"
#include <QDebug>
#include <QFileInfo>

ZipReader::ZipReader(const QString &zipPath)
    : m_zipPath(zipPath)
    , m_zip(nullptr)
    , m_isOpen(false)
    , m_needsPassword(false)
{
}

ZipReader::~ZipReader()
{
    close();
}

bool ZipReader::open(QString *errorString)
{
    m_mutex.lock();

    if (m_isOpen) {
        m_mutex.unlock();
        return true;
    }

    int error;
    m_zip = zip_open(m_zipPath.toUtf8().constData(), ZIP_RDONLY, &error);

    if (!m_zip) {
        if (errorString) {
            *errorString = QString("无法打开 ZIP 文件: %1").arg(m_zipPath);
        }
        m_mutex.unlock();
        return false;
    }

    // 检查是否需要密码 - 尝试读取第一个文件
    zip_int64_t numEntries = zip_get_num_entries(m_zip, 0);
    if (numEntries > 0) {
        for (zip_int64_t i = 0; i < numEntries; i++) {
            const char *name = zip_get_name(m_zip, i, 0);
            if (name && name[0] != '\0' && name[strlen(name) - 1] != '/') {
                // 尝试打开第一个非目录文件
                zip_file_t *zf = zip_fopen_index(m_zip, i, 0);
                if (!zf) {
                    int zipError = zip_get_error(m_zip)->zip_err;
                    if (zipError == ZIP_ER_WRONGPASSWD || zipError == ZIP_ER_ENCRNOTSUPP) {
                        m_needsPassword = true;
                        if (errorString) {
                            *errorString = "ZIP 文件需要密码";
                        }
                        zip_close(m_zip);
                        m_zip = nullptr;
                        m_mutex.unlock();
                        return false;
                    }
                } else {
                    zip_fclose(zf);
                }
                break;
            }
        }
    }

    m_isOpen = true;
    scanEntries();
    m_mutex.unlock();
    return true;
}

bool ZipReader::openWithPassword(const QString &password, QString *errorString)
{
    m_mutex.lock();

    if (m_isOpen) {
        zip_close(m_zip);
        m_zip = nullptr;
        m_isOpen = false;
    }

    int error;
    m_zip = zip_open(m_zipPath.toUtf8().constData(), ZIP_RDONLY, &error);

    if (!m_zip) {
        if (errorString) {
            *errorString = QString("无法打开 ZIP 文件: %1").arg(m_zipPath);
        }
        m_mutex.unlock();
        return false;
    }

    // 设置默认密码
    if (zip_set_default_password(m_zip, password.toUtf8().constData()) < 0) {
        if (errorString) {
            *errorString = "设置密码失败";
        }
        zip_close(m_zip);
        m_zip = nullptr;
        m_mutex.unlock();
        return false;
    }

    // 验证密码是否正确
    zip_int64_t numEntries = zip_get_num_entries(m_zip, 0);
    if (numEntries > 0) {
        for (zip_int64_t i = 0; i < numEntries; i++) {
            const char *name = zip_get_name(m_zip, i, 0);
            if (name && name[0] != '\0' && name[strlen(name) - 1] != '/') {
                zip_file_t *zf = zip_fopen_index(m_zip, i, 0);
                if (!zf) {
                    int zipError = zip_get_error(m_zip)->zip_err;
                    if (zipError == ZIP_ER_WRONGPASSWD) {
                        if (errorString) {
                            *errorString = "密码不正确";
                        }
                        zip_close(m_zip);
                        m_zip = nullptr;
                        m_mutex.unlock();
                        return false;
                    }
                } else {
                    zip_fclose(zf);
                }
                break;
            }
        }
    }

    m_password = password;
    m_needsPassword = false;
    m_isOpen = true;
    scanEntries();
    m_mutex.unlock();
    return true;
}

void ZipReader::close()
{
    m_mutex.lock();
    if (m_zip) {
        if (m_isOpen) {
            zip_close(m_zip);
        }
        m_zip = nullptr;
    }
    m_isOpen = false;
    m_entries.clear();
    m_mutex.unlock();
}

void ZipReader::scanEntries()
{
    m_entries.clear();

    if (!m_zip || !m_isOpen) {
        return;
    }

    zip_int64_t numEntries = zip_get_num_entries(m_zip, 0);
    for (zip_int64_t i = 0; i < numEntries; i++) {
        const char *name = zip_get_name(m_zip, i, 0);
        if (!name) continue;

        ZipEntry entry;
        entry.filePath = QString::fromUtf8(name);
        entry.fileName = QFileInfo(entry.filePath).fileName();
        entry.isDir = (name[0] != '\0' && name[strlen(name) - 1] == '/');

        // 获取文件信息
        zip_stat_t st;
        if (zip_stat_index(m_zip, i, 0, &st) == 0) {
            entry.fileSize = st.size;
            if (st.valid & ZIP_STAT_MTIME) {
                entry.dateTime = QDateTime::fromSecsSinceEpoch(st.mtime);
            }
        }

        if (!entry.isDir) {
            m_entries.append(entry);
        }
    }

    // 按文件名排序
    std::sort(m_entries.begin(), m_entries.end(), [](const ZipEntry &a, const ZipEntry &b) {
        return a.filePath < b.filePath;
    });
}

QList<ZipReader::ZipEntry> ZipReader::getMediaEntries() const
{
    QList<ZipEntry> mediaEntries;

    for (const ZipEntry &entry : m_entries) {
        if (isMediaFile(entry.fileName)) {
            mediaEntries.append(entry);
        }
    }

    return mediaEntries;
}

QByteArray ZipReader::readFile(const QString &filePath, QString *errorString)
{
    m_mutex.lock();

    QByteArray data;

    if (!m_zip || !m_isOpen) {
        if (errorString) {
            *errorString = "ZIP 文件未打开";
        }
        m_mutex.unlock();
        return data;
    }

    // 查找文件索引
    zip_int64_t index = -1;
    zip_int64_t numEntries = zip_get_num_entries(m_zip, 0);
    for (zip_int64_t i = 0; i < numEntries; i++) {
        const char *name = zip_get_name(m_zip, i, 0);
        if (name && filePath == QString::fromUtf8(name)) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        if (errorString) {
            *errorString = QString("找不到文件: %1").arg(filePath);
        }
        m_mutex.unlock();
        return data;
    }

    // 获取文件信息以获取大小
    zip_stat_t st;
    if (zip_stat_index(m_zip, index, 0, &st) != 0) {
        if (errorString) {
            *errorString = QString("无法获取文件信息: %1").arg(filePath);
        }
        m_mutex.unlock();
        return data;
    }

    // 打开并读取文件
    zip_file_t *zf = zip_fopen_index(m_zip, index, 0);
    if (!zf) {
        if (errorString) {
            *errorString = QString("无法打开文件: %1, 错误: %2")
                .arg(filePath)
                .arg(zip_error_strerror(zip_get_error(m_zip)));
        }
        m_mutex.unlock();
        return data;
    }

    // 读取文件内容
    data.resize(st.size);
    zip_int64_t bytesRead = zip_fread(zf, data.data(), st.size);
    if (bytesRead < 0) {
        if (errorString) {
            *errorString = QString("读取文件失败: %1").arg(filePath);
        }
        data.clear();
    } else {
        data.resize(bytesRead);
    }

    zip_fclose(zf);
    m_mutex.unlock();
    return data;
}

bool ZipReader::isImageFile(const QString &fileName)
{
    static const QStringList imageExts = {
        "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"
    };
    QString ext = QFileInfo(fileName).suffix().toLower();
    return imageExts.contains(ext);
}

bool ZipReader::isVideoFile(const QString &fileName)
{
    static const QStringList videoExts = {
        "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"
    };
    QString ext = QFileInfo(fileName).suffix().toLower();
    return videoExts.contains(ext);
}

bool ZipReader::isMediaFile(const QString &fileName)
{
    return isImageFile(fileName) || isVideoFile(fileName);
}

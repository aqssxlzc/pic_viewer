#ifndef ZIPREADER_H
#define ZIPREADER_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QMutex>
#include <QDateTime>

// 使用 libzip 库
#include <zip.h>

/**
 * ZIP 文件读取器
 * 支持读取 ZIP 压缩包内的图片和视频文件，不解压
 * 支持密码保护的 ZIP 文件
 */
class ZipReader
{
public:
    struct ZipEntry {
        QString fileName;       // ZIP 内的文件名
        QString filePath;       // ZIP 内的完整路径
        qint64 fileSize;        // 文件大小
        bool isDir;             // 是否是目录
        QDateTime dateTime;     // 修改时间
    };

    ZipReader(const QString &zipPath);
    ~ZipReader();

    // 打开 ZIP 文件，如果需要密码会返回 false 并设置 needsPassword
    bool open(QString *errorString = nullptr);

    // 使用密码打开
    bool openWithPassword(const QString &password, QString *errorString = nullptr);

    // 关闭 ZIP 文件
    void close();

    // 是否需要密码
    bool needsPassword() const { return m_needsPassword; }

    // 是否已打开
    bool isOpen() const { return m_isOpen; }

    // 获取 ZIP 内的所有媒体文件（图片和视频）
    QList<ZipEntry> getMediaEntries() const;

    // 读取文件内容到内存
    QByteArray readFile(const QString &filePath, QString *errorString = nullptr);

    // 获取 ZIP 文件路径
    QString zipPath() const { return m_zipPath; }

    // 判断是否是支持的媒体文件
    static bool isMediaFile(const QString &fileName);
    static bool isImageFile(const QString &fileName);
    static bool isVideoFile(const QString &fileName);

private:
    QString m_zipPath;
    zip_t *m_zip;
    bool m_isOpen;
    bool m_needsPassword;
    QString m_password;
    mutable QMutex m_mutex;

    QList<ZipEntry> m_entries;

    void scanEntries();
};

#endif // ZIPREADER_H

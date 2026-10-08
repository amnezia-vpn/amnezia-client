#include "logsController.h"

#include <QFile>
#include <QFileInfo>

#ifdef AMNEZIA_HAS_QZIPWRITER
    #include <QBuffer>
    #include <QtCore/private/qzipwriter_p.h>
#else
    #include <QDateTime>

    #include <array>
#endif

#include <limits>

#include "logger.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "core/utils/swiftBridge.h"
#elif defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

#ifndef AMNEZIA_HAS_QZIPWRITER
namespace
{
    quint32 crc32(const QByteArray &data)
    {
        static const std::array<quint32, 256> table = [] {
            std::array<quint32, 256> result {};
            for (quint32 i = 0; i < 256; ++i) {
                quint32 value = i;
                for (int bit = 0; bit < 8; ++bit) {
                    value = (value & 1) ? 0xEDB88320u ^ (value >> 1) : value >> 1;
                }
                result[i] = value;
            }
            return result;
        }();

        quint32 crc = 0xFFFFFFFFu;
        for (const char byte : data) {
            crc = table[(crc ^ static_cast<uchar>(byte)) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFFu;
    }

    void put16(QByteArray &out, quint16 value)
    {
        out.append(static_cast<char>(value & 0xFF));
        out.append(static_cast<char>(value >> 8));
    }

    void put32(QByteArray &out, quint32 value)
    {
        put16(out, static_cast<quint16>(value & 0xFFFF));
        put16(out, static_cast<quint16>(value >> 16));
    }

    QByteArray storedZip(const QList<QPair<QString, QByteArray>> &entries)
    {
        const QDateTime now = QDateTime::currentDateTime();
        const quint16 dosTime = (now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2);
        const quint16 dosDate = ((now.date().year() - 1980) << 9) | (now.date().month() << 5) | now.date().day();

        QByteArray archive;
        QByteArray centralDirectory;
        for (const auto &[name, data] : entries) {
            const QByteArray fileName = name.toUtf8();
            const quint32 offset = static_cast<quint32>(archive.size());
            const quint32 size = static_cast<quint32>(data.size());

            QByteArray fields;
            put16(fields, 10);
            put16(fields, 0);
            put16(fields, 0);
            put16(fields, dosTime);
            put16(fields, dosDate);
            put32(fields, crc32(data));
            put32(fields, size);
            put32(fields, size);
            put16(fields, static_cast<quint16>(fileName.size()));
            put16(fields, 0);

            put32(archive, 0x04034b50);
            archive.append(fields).append(fileName).append(data);

            put32(centralDirectory, 0x02014b50);
            put16(centralDirectory, 20);
            centralDirectory.append(fields);
            put16(centralDirectory, 0);
            put16(centralDirectory, 0);
            put16(centralDirectory, 0);
            put32(centralDirectory, 0);
            put32(centralDirectory, offset);
            centralDirectory.append(fileName);
        }

        const quint32 centralDirectoryOffset = static_cast<quint32>(archive.size());
        archive.append(centralDirectory);

        put32(archive, 0x06054b50);
        put16(archive, 0);
        put16(archive, 0);
        put16(archive, static_cast<quint16>(entries.size()));
        put16(archive, static_cast<quint16>(entries.size()));
        put32(archive, static_cast<quint32>(centralDirectory.size()));
        put32(archive, centralDirectoryOffset);
        put16(archive, 0);
        return archive;
    }
}
#endif

QStringList LogsController::files(Stream stream)
{
#if defined(AMNEZIA_DESKTOP)
    if (stream == Stream::App) {
        return { Logger::userLogsFilePath() };
    }
    return { Logger::serviceLogsFilePath() };
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
    if (stream == Stream::App) {
        return { QString::fromStdString(SWIFT_BRIDGE_NAMESPACE::swiftAppLogPath()), Logger::userLogsFilePath() };
    }
    return { QString::fromStdString(SWIFT_BRIDGE_NAMESPACE::swiftTunnelLogPath()) };
#elif defined(Q_OS_ANDROID)
    return AndroidController::logFiles(static_cast<int>(stream));
#endif
}

qint64 LogsController::size(Stream stream)
{
    qint64 total = 0;
    for (const QString &path : files(stream)) {
        total += fileSize(path);
    }
    return total;
}

qint64 LogsController::fileSize(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.size() : 0;
}

QString LogsController::readTail(Stream stream, qint64 *newestFileEnd, qint64 maxBytes)
{
    return QString::fromUtf8(read(stream, maxBytes, newestFileEnd));
}

QByteArray LogsController::readAll(Stream stream)
{
    return read(stream, std::numeric_limits<qint64>::max());
}

QByteArray LogsController::read(Stream stream, qint64 maxBytes, qint64 *newestFileEnd)
{
    QByteArray data = tailOfFiles(files(stream), maxBytes, newestFileEnd);
#if defined(Q_OS_ANDROID)
    if (stream == Stream::App && maxBytes == std::numeric_limits<qint64>::max()) {
        data.prepend(AndroidController::deviceInfo().toUtf8() + '\n');
    }
#endif
    return data;
}

void LogsController::clear(Stream stream)
{
#if defined(AMNEZIA_DESKTOP)
    if (stream == Stream::App) {
        Logger::clearLogs(false);
    } else {
        Logger::clearServiceLogs();
    }
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
    if (stream == Stream::App) {
        Logger::clearLogs(false);
        QFile::resize(QString::fromStdString(SWIFT_BRIDGE_NAMESPACE::swiftAppLogPath()), 0);
    } else {
        QFile::resize(QString::fromStdString(SWIFT_BRIDGE_NAMESPACE::swiftTunnelLogPath()), 0);
    }
#elif defined(Q_OS_ANDROID)
    AndroidController::clearLogStream(static_cast<int>(stream));
#endif
}

QByteArray LogsController::zipAll()
{
    return zipFiles({ { QStringLiteral("app.log"), readAll(Stream::App) },
                      { QStringLiteral("tunnel.log"), readAll(Stream::Tunnel) } });
}

QByteArray LogsController::zipFiles(const QList<QPair<QString, QByteArray>> &entries)
{
#ifdef AMNEZIA_HAS_QZIPWRITER
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QZipWriter zip(&buffer);
    zip.setCompressionPolicy(QZipWriter::AlwaysCompress);
    for (const auto &[name, data] : entries) {
        zip.addFile(name, data);
    }
    zip.close();

    return buffer.data();
#else
    return storedZip(entries);
#endif
}

QByteArray LogsController::tailOfFiles(const QStringList &paths, qint64 maxBytes, qint64 *newestFileEnd)
{
    if (newestFileEnd) {
        *newestFileEnd = 0;
    }

    QList<QPair<QString, QByteArray>> chunks;
    qint64 remaining = maxBytes;

    for (auto it = paths.crbegin(); it != paths.crend() && remaining > 0; ++it) {
        QFile file(*it);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const qint64 fileSize = file.size();
        QByteArray chunk;
        if (fileSize <= remaining) {
            chunk = file.read(fileSize);
            remaining -= fileSize;
        } else {
            file.seek(fileSize - remaining - 1);
            chunk = file.read(remaining + 1);
            const qsizetype newline = chunk.indexOf('\n');
            qsizetype start = newline + 1;
            if (newline < 0) {
                start = 1;
                while (start < chunk.size() && (static_cast<uchar>(chunk.at(start)) & 0xC0) == 0x80) {
                    ++start;
                }
            }
            chunk = chunk.mid(start);
            remaining = 0;
        }

        if (newestFileEnd && it == paths.crbegin()) {
            *newestFileEnd = file.pos();
        }

        if (!chunk.isEmpty()) {
            chunks.prepend({ QFileInfo(*it).fileName(), chunk });
        }
    }

    if (chunks.size() == 1) {
        return chunks.first().second;
    }

    QByteArray result;
    for (const auto &[fileName, chunk] : chunks) {
        result += "----- " + fileName.toUtf8() + " -----\n" + chunk;
        if (!chunk.endsWith('\n')) {
            result += '\n';
        }
    }
    return result;
}

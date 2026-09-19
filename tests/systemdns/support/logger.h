#pragma once
#include <QDebug>

// Keep test diagnostics on stderr instead of using the app's IPC/file logger.
class Logger {
public:
    explicit Logger(const QString&) {}
    QDebug debug() { return qDebug(); }
    QDebug info() { return qInfo(); }
    QDebug warning() { return qWarning(); }
    QDebug error() { return qCritical(); }
};

#include "vpnUrlHandler.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#if defined(Q_OS_WIN)
#include <QDesktopServices>
#include <QUrl>

#include <windows.h>
#include <shlwapi.h>

namespace {
const QString kClassesKey = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\vpn");
const QString kCommandKey = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\vpn\\shell\\open\\command");
const QString kUrlName = QStringLiteral("URL:AmneziaVPN");

QString appExe()
{
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

QString openCommand()
{
    return QStringLiteral("\"%1\" \"%2\"").arg(appExe(), QStringLiteral("%1"));
}

QString effectiveHandlerExe()
{
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD size = ARRAYSIZE(buf);
    const HRESULT hr = AssocQueryStringW(ASSOCF_IS_PROTOCOL, ASSOCSTR_EXECUTABLE, L"vpn", nullptr, buf, &size);
    if (FAILED(hr)) {
        return {};
    }
    return QString::fromWCharArray(buf);
}
} // namespace

VpnUrlHandler::Status VpnUrlHandler::status()
{
    const QString handler = effectiveHandlerExe();
    if (handler.isEmpty()) {
        return Status::NotRegistered;
    }

    if (QFileInfo(handler).canonicalFilePath().compare(QFileInfo(appExe()).canonicalFilePath(), Qt::CaseInsensitive) != 0) {
        return Status::OtherHandler;
    }

    QSettings vpnKey(kClassesKey, QSettings::NativeFormat);
    if (!vpnKey.contains(QStringLiteral("URL Protocol"))) {
        return Status::OtherHandler;
    }
    return Status::IsDefault;
}

bool VpnUrlHandler::setAsDefault()
{
    QSettings vpnKey(kClassesKey, QSettings::NativeFormat);
    vpnKey.setValue(QStringLiteral("."), kUrlName);
    vpnKey.setValue(QStringLiteral("URL Protocol"), QString());
    vpnKey.sync();

    QSettings cmdKey(kCommandKey, QSettings::NativeFormat);
    cmdKey.setValue(QStringLiteral("."), openCommand());
    cmdKey.sync();

    if (status() == Status::IsDefault) {
        return true;
    }

    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
    return false;
}

void VpnUrlHandler::unregister()
{
    QSettings cmdKey(kCommandKey, QSettings::NativeFormat);
    if (cmdKey.value(QStringLiteral(".")).toString().compare(openCommand(), Qt::CaseInsensitive) != 0) {
        return;
    }

    QSettings classesKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes"), QSettings::NativeFormat);
    classesKey.remove(QStringLiteral("vpn"));
}

#elif defined(Q_OS_MACX)

#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

namespace {
const QString kDesktopFile = QStringLiteral("AmneziaVPN.desktop");
const QString kMimeType = QStringLiteral("x-scheme-handler/vpn");

bool runXdgMime(const QStringList &args, QString *output = nullptr)
{
    QProcess process;
    process.start(QStringLiteral("xdg-mime"), args);
    if (!process.waitForFinished(3000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return false;
    }
    if (output) {
        *output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    }
    return true;
}
} // namespace

VpnUrlHandler::Status VpnUrlHandler::status()
{
    if (QStandardPaths::findExecutable(QStringLiteral("xdg-mime")).isEmpty()) {
        return Status::IsDefault;
    }

    QString handler;
    if (!runXdgMime({ QStringLiteral("query"), QStringLiteral("default"), kMimeType }, &handler) || handler.isEmpty()) {
        return Status::NotRegistered;
    }
    return handler == kDesktopFile ? Status::IsDefault : Status::OtherHandler;
}

bool VpnUrlHandler::setAsDefault()
{
    return runXdgMime({ QStringLiteral("default"), kDesktopFile, kMimeType }) && status() == Status::IsDefault;
}

void VpnUrlHandler::unregister()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/mimeapps.list");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    file.close();

    const QString prefix = kMimeType + QLatin1Char('=');
    QStringList result;
    bool changed = false;
    for (const QString &line : lines) {
        if (!line.startsWith(prefix)) {
            result << line;
            continue;
        }
        QStringList apps = line.mid(prefix.size()).split(QLatin1Char(';'), Qt::SkipEmptyParts);
        if (apps.removeAll(kDesktopFile) == 0) {
            result << line;
            continue;
        }
        changed = true;
        if (!apps.isEmpty()) {
            result << prefix + apps.join(QLatin1Char(';')) + QLatin1Char(';');
        }
    }

    if (changed && file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        file.write(result.join(QLatin1Char('\n')).toUtf8());
    }
}

#else

VpnUrlHandler::Status VpnUrlHandler::status()
{
    return Status::IsDefault;
}

bool VpnUrlHandler::setAsDefault()
{
    return true;
}

void VpnUrlHandler::unregister()
{
}

#endif

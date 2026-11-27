#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/utils/utilities.h"

#ifdef Q_OS_WINDOWS
QString Utils::getNextDriverLetter()
{
    DWORD drivesBitmask = GetLogicalDrives();
    if (drivesBitmask == 0) {
        DWORD error = GetLastError();
        qDebug() << "GetLogicalDrives failed. Error code:" << error;
        return "";
    }

    QString letters = "FGHIJKLMNOPQRSTUVWXYZ";
    QString availableLetter;

    for (int i = letters.size() - 1; i >= 0; --i) {
        QChar letterChar = letters.at(i);
        int driveIndex = letterChar.toLatin1() - 'A';

        if ((drivesBitmask & (1 << driveIndex)) == 0) {
            availableLetter = letterChar;
            break;
        }
    }

    if (availableLetter.isEmpty()) {
        qDebug() << "Can't find free drive letter";
        return "";
    }

    return availableLetter;
}
#endif

QString Utils::getRandomString(int len)
{
    const QString possibleCharacters = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    QString randomString;

    for (int i = 0; i < len; ++i) {
        randomString.append(possibleCharacters.at(QRandomGenerator::system()->bounded(possibleCharacters.length())));
    }

    return randomString;
}

QString Utils::VerifyJsonString(const QString &source)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(source.toUtf8(), &error);
    Q_UNUSED(doc)

    if (error.error == QJsonParseError::NoError) {
        return "";
    } else {
        qDebug() << "WARNING: Json parse returns: " + error.errorString();
        return error.errorString();
    }
}

QJsonObject Utils::JsonFromString(const QString &string)
{
    auto removeComment = string.trimmed();
    if (removeComment != string.trimmed()) {
        qDebug() << "Some comments have been removed from the json.";
    }
    QJsonDocument doc = QJsonDocument::fromJson(removeComment.toUtf8());
    return doc.object();
}

QString Utils::SafeBase64Decode(QString string)
{
    QByteArray ba = string.replace(QChar('-'), QChar('+')).replace(QChar('_'), QChar('/')).toUtf8();
    return QByteArray::fromBase64(ba, QByteArray::Base64Option::OmitTrailingEquals);
}

QString Utils::JsonToString(const QJsonObject &json, QJsonDocument::JsonFormat format)
{
    QJsonDocument doc;
    doc.setObject(json);
    return doc.toJson(format);
}

QString Utils::JsonToString(const QJsonArray &array, QJsonDocument::JsonFormat format)
{
    QJsonDocument doc;
    doc.setArray(array);
    return doc.toJson(format);
}

bool Utils::initializePath(const QString &path)
{
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning().noquote() << QString("Cannot initialize path: '%1'").arg(path);
        return false;
    }
    return true;
}

bool Utils::createEmptyFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::executable(const QString &baseName, bool absPath)
{
    QString ext;
#ifdef Q_OS_WIN
    ext = ".exe";
#endif
    const QString fileName = baseName + ext;
    if (!absPath) {
        return fileName;
    }
    return QCoreApplication::applicationDirPath() + "/" + fileName;
}

QString Utils::usrExecutable(const QString &baseName)
{
    if (QFileInfo::exists("/usr/sbin/" + baseName))
        return ("/usr/sbin/" + baseName);
    else
        return ("/usr/bin/" + baseName);
}

bool Utils::processIsRunning(const QString &fileName, const bool fullFlag)
{
#ifdef Q_OS_WIN
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "Utils::processIsRunning error CreateToolhelp32Snapshot";
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        qWarning() << "Utils::processIsRunning error Process32FirstW";
        return false;
    }

    do {
        QString exeFile = QString::fromWCharArray(pe32.szExeFile);

        if (exeFile.compare(fileName, Qt::CaseInsensitive) == 0) {
            CloseHandle(hSnapshot);
            return true;
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return false;

#elif defined(Q_OS_IOS) || defined(Q_OS_ANDROID) || defined(MACOS_NE)
    return false;
#else
    QProcess process;
    QStringList arguments;
    if (fullFlag) {
        arguments << "-f";
    }
    arguments << fileName;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("pgrep", arguments);
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit) {
        if (fullFlag) {
            return (process.readLine().toUInt() > 0);
        } else {
            return (process.readAll().toUInt() > 0);
        }
    }
    return false;
#endif
}

#ifdef Q_OS_WIN
// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
bool Utils::signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent)
{
    bool success = false;
    DWORD thisConsoleId = GetCurrentProcessId();
    // Leave current console if it exists
    // (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
    bool consoleDetached = (FreeConsole() != FALSE);

    if (AttachConsole(dwProcessId) != FALSE) {
        // Add a fake Ctrl-C handler for avoid instant kill is this console
        // WARNING: do not revert it or current program will be also killed
        SetConsoleCtrlHandler(nullptr, true);
        success = (GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE);
        FreeConsole();
    }

    if (consoleDetached) {
        // Create a new console if previous was deleted by OS
        if (AttachConsole(thisConsoleId) == FALSE) {
            int errorCode = GetLastError();
            if (errorCode == 31) // 31=ERROR_GEN_FAILURE
            {
                AllocConsole();
            }
        }
    }
    return success;
}

#endif

void Utils::logException(const std::exception &e)
{
    qCritical() << e.what();
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &nested) {
        logException(nested);
    } catch (...) {}
}

void Utils::logException(const std::exception_ptr &eptr)
{
    try {
        if (eptr) std::rethrow_exception(eptr);
    } catch (const std::exception &e) {
        logException(e);
    } catch (...) {}
}

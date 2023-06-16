#ifndef UTILITIES_H
#define UTILITIES_H

#include <QFileDialog>
#include <QRegExp>
#include <QRegularExpression>
#include <QString>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

class Utils : public QObject {
    Q_OBJECT

public:
    static QString getRandomString(int len);

    static QString executable(const QString& baseName, bool absPath);
    static QString usrExecutable(const QString& baseName);
    static QString systemLogPath();
    static bool createEmptyFile(const QString& path);
    static bool initializePath(const QString& path);

    static QString getIPAddress(const QString& host);
    static QString getStringBetween(const QString& s, const QString& a, const QString& b);
    static bool checkIPv4Format(const QString& ip);
    static bool checkIpSubnetFormat(const QString& ip);
    static QRegularExpression ipAddressRegExp() { return QRegularExpression("^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}$"); }
    static QRegularExpression ipAddressPortRegExp() { return QRegularExpression("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
        "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\\:[0-9]{1,5}){0,1}$"); }

    static QRegExp ipAddressWithSubnetRegExp() { return QRegExp("(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
        "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\\/[0-9]{1,2}){0,1}"); }

    static QRegExp ipNetwork24RegExp() { return QRegExp("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
        "0$"); }

    static QRegExp ipPortRegExp() { return QRegExp("^()([1-9]|[1-5]?[0-9]{2,4}|6[1-4][0-9]{3}|65[1-4][0-9]{2}|655[1-2][0-9]|6553[1-5])$"); }

    static QRegExp domainRegExp() { return QRegExp("(((?!\\-))(xn\\-\\-)?[a-z0-9\\-_]{0,61}[a-z0-9]{1,1}\\.)*(xn\\-\\-)?([a-z0-9\\-]{1,61}|[a-z0-9\\-]{1,30})\\.[a-z]{2,}"); }
    static bool processIsRunning(const QString& fileName);
    static void killProcessByName(const QString &name);

    static QString netMaskFromIpWithSubnet(const QString ip);
    static QString ipAddressFromIpWithSubnet(const QString ip);

    static QStringList summarizeRoutes(const QStringList &ips, const QString cidr);

    static QString openVpnExecPath();
    static QString wireguardExecPath();
    static QString certUtilPath();

    static void saveFile(const QString &fileExtension,
                         const QString &caption,
                         const QString &fileName,
                         const QString &data);

    static QString getFileName(QWidget *parent = nullptr,
                               const QString &caption = QString(),
                               const QString &dir = QString(),
                               const QString &filter = QString(),
                               QString *selectedFilter = nullptr,
                               QFileDialog::Options options = QFileDialog::Options());

#ifdef Q_OS_WIN
    static bool signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent);
#endif
};

#endif // UTILITIES_H

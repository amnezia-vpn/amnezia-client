#include "migrations.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "version.h"

Migrations::Migrations(QObject *parent)
    : QObject{parent}
{
    QString version(APP_MAJOR_VERSION);

    QStringList versionDigits = version.split(".");

    if (versionDigits.size() >= 3) {
        currentMajor = versionDigits[0].toInt();
        currentMinor = versionDigits[1].toInt();
        currentMicro = versionDigits[2].toInt();
    }

    if (versionDigits.size() == 4) {
        currentPatch = versionDigits[3].toInt();
    }
}

void Migrations::doMigrations()
{
    if (currentMajor == 3) {
        migrateV3();
    }
}

void Migrations::migrateV3()
{
#ifdef Q_OS_ANDROID
    qDebug() << "Migration to V3 on Android...";

    QString packageName = "org.amnezia.vpn";

    QDir dir(".");
    QString currentDir = dir.absolutePath();

    int packageNameIndex = currentDir.indexOf(packageName);

    if (packageNameIndex == -1) {
        return;
    }

    QString rootLocation = currentDir.left(packageNameIndex + packageName.size());

    if (rootLocation.isEmpty()) {
        return;
    }

    QString location = rootLocation + "/files/.config/VPNNaruzhu.ORG/VPNNaruzhu.conf";

    QFile oldConfig(location);

    if (oldConfig.exists()) {
        QString newConfigPath = rootLocation + "/files/settings";

        QDir newConfigDir(newConfigPath);

        newConfigPath += "/VPNNaruzhu.ORG";

        bool mkPathRes = newConfigDir.mkpath(newConfigPath);

        if (!mkPathRes) {
            return;
        }

        QFile newConfigFile(newConfigPath + "/VPNNaruzhu.conf");

        if (!newConfigFile.exists()) {
            bool cpResult = QFile::copy(oldConfig.fileName(), newConfigFile.fileName());
            if (cpResult) {
                oldConfig.remove();
                QDir oldConfigDir(rootLocation + "/files/.config");
                oldConfigDir.rmdir("VPNNaruzhu.ORG");
            }
        }
    }
#endif
}

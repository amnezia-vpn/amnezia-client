#include "ssh_configurator.h"

#include <QDebug>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <qtimer.h>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "core/server_defs.h"
#include "utilities.h"

SshConfigurator::SshConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : ConfiguratorBase(settings, parent)
{
}

QString SshConfigurator::convertOpenSShKey(const QString &key)
{
#ifndef Q_OS_IOS
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    QTemporaryFile tmp;
    #ifdef QT_DEBUG
    tmp.setAutoRemove(false);
    #endif
    tmp.open();
    tmp.write(key.toUtf8());
    tmp.close();

    // ssh-keygen -p -P "" -N "" -m pem -f id_ssh

    #ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ssh-keygen.exe -p -P \"\" -N \"\" -m pem -f \"%1\"\"").arg(tmp.fileName()));
    #else
    p.setProgram("ssh-keygen");
    p.setArguments(QStringList() << "-p"
                                 << "-P"
                                 << ""
                                 << "-N"
                                 << ""
                                 << "-m"
                                 << "pem"
                                 << "-f" << tmp.fileName());
    #endif

    p.start();
    p.waitForFinished();

    qDebug().noquote() << "OpenVpnConfigurator::convertOpenSShKey" << p.exitCode() << p.exitStatus() << p.readAll();

    tmp.open();

    return tmp.readAll();
#else
    return key;
#endif
}

void SshConfigurator::openSshTerminal(const ServerCredentials &credentials)
{
#ifndef Q_OS_IOS
    QProcess *p = new QProcess();
    p->setProcessChannelMode(QProcess::SeparateChannels);

    #ifdef Q_OS_WIN
    p->setProcessEnvironment(prepareEnv());
    p->setProgram(qApp->applicationDirPath() + "\\cygwin\\putty.exe");

    if (credentials.secretData.contains("PRIVATE KEY")) {
        // todo: connect by key
        //        p->setNativeArguments(QString("%1@%2")
        //            .arg(credentials.userName).arg(credentials.hostName).arg(credentials.secretData));
    } else {
        p->setNativeArguments(
                QString("%1@%2 -pw %3").arg(credentials.userName).arg(credentials.hostName).arg(credentials.secretData));
    }
    #else
    p->setProgram("/bin/bash");
    #endif

    p->startDetached();
#endif
}

QProcessEnvironment SshConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#ifdef Q_OS_WIN
    pathEnvVar.clear();
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\cygwin;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn;");
#elif defined(Q_OS_MACX)
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#endif

    env.insert("PATH", pathEnvVar);
    // qDebug().noquote() << "ENV PATH" << pathEnvVar;
    return env;
}

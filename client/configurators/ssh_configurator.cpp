#include "ssh_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QThread>
#include <QObject>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <qtimer.h>

#include "core/server_defs.h"
#include "utils.h"

using namespace QSsh;


QString SshConfigurator::convertOpenSShKey(const QString &key)
{
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
    p.setArguments(QStringList() << "-p" << "-P" << "" << "-N" << "" << "-m" << "pem" << "-f" << tmp.fileName());
#endif

    p.start();
    p.waitForFinished();

    qDebug().noquote() << "OpenVpnConfigurator::convertOpenSShKey" << p.exitCode() << p.exitStatus() << p.readAll();

    tmp.open();

    return tmp.readAll();
}

void SshConfigurator::openSshTerminal(const ServerCredentials &credentials)
{
    QProcess *p = new QProcess();
    p->setReadChannelMode(QProcess::SeparateChannels);

#ifdef Q_OS_WIN
    p->setProcessEnvironment(prepareEnv());
    p->setProgram(qApp->applicationDirPath() + "\\cygwin\\putty.exe");

    if (credentials.password.contains("PRIVATE KEY")) {
        // todo: connect by key
//        p->setNativeArguments(QString("%1@%2")
//            .arg(credentials.userName).arg(credentials.hostName).arg(credentials.password));
    }
    else {
        p->setNativeArguments(QString("%1@%2 -pw %3")
            .arg(credentials.userName).arg(credentials.hostName).arg(credentials.password));
    }
#else
    p->setProgram("/bin/bash");
#endif

    p->startDetached();
}

QProcessEnvironment SshConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#ifdef Q_OS_WIN
    pathEnvVar.clear();
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\cygwin;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn;");
#else
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#endif

    env.insert("PATH", pathEnvVar);
    //qDebug().noquote() << "ENV PATH" << pathEnvVar;
    return env;
}

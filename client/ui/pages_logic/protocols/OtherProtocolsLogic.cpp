#include <QDesktopServices>
#include <QTimer>
#include <QProcess>
#include <QStorageInfo>

#include "OtherProtocolsLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"
#include "utils.h"

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif

using namespace amnezia;
using namespace PageEnumNS;

OtherProtocolsLogic::OtherProtocolsLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent)
{

}

OtherProtocolsLogic::~OtherProtocolsLogic()
{
#ifdef Q_OS_WINDOWS
    for (QProcess *p: m_sftpMountProcesses) {
        if (p) Utils::signalCtrl(p->processId(), CTRL_C_EVENT);
        if (p) p->kill();
        if (p) p->waitForFinished();
        if (p) delete p;
    }
#endif
}

void OtherProtocolsLogic::updateProtocolPage(const QJsonObject &config, DockerContainer container, bool haveAuthData)
{
    set_labelTftpUserNameText(config.value(config_key::userName).toString());
    set_labelTftpPasswordText(config.value(config_key::password).toString(protocols::sftp::defaultUserName));
    set_labelTftpPortText(config.value(config_key::port).toString());

    set_labelTorWebSiteAddressText(config.value(config_key::site).toString());
}

//QJsonObject OtherProtocolsLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
//{

//}


void OtherProtocolsLogic::onPushButtonSftpMountDriveClicked()
{
#ifdef Q_OS_WINDOWS
    QProcess drivesProc;
    drivesProc.start("wmic logicaldisk get caption");
    drivesProc.waitForFinished();
    QString drives = drivesProc.readAll();
    qDebug() << drives;


    QString letters = "CFGHIJKLMNOPQRSTUVWXYZ";
    QString letter;
    for (int i = letters.size() - 1; i > 0; i--) {
        letter = letters.at(i);
        if (!drives.contains(letter + ":")) break;
    }
    if (letter == "C:") {
        // set err info
        qDebug() << "Can't find free drive letter";
        return;
    }


    set_pushButtonSftpMountEnabled(false);
    QProcess *p = new QProcess;
    m_sftpMountProcesses.append(p);
    p->setProcessChannelMode(QProcess::MergedChannels);

    connect(p, &QProcess::readyRead, this, [this, p, letter](){
        QString s = p->readAll();
        if (s.contains("The service sshfs has been started")) {
            QDesktopServices::openUrl(QUrl("file:///" + letter + ":"));
            set_pushButtonSftpMountEnabled(true);
        }
    });

//    QString cmd = QString("net use \\\\sshfs\\%1@51.77.32.168!%2 /USER:%1 %3")
//            .arg(labelTftpUserNameText())
//            .arg(labelTftpPortText())
//            .arg(labelTftpPasswordText());

    p->setProgram("C:\\Program Files1\\SSHFS-Win\\bin\\sshfs.exe");

    QString host = m_settings.serverCredentials(uiLogic()->selectedServerIndex).hostName;
    QString args = QString(
                        "%1@%2:/ %3: "
                        "-o port=%4 "
                        "-f "
                        "-o reconnect"
                        "-orellinks "
                        "-ofstypename=SSHFS "
                        "-o ssh_command=/usr/bin/ssh.exe "
                        "-oUserKnownHostsFile=/dev/null "
                        "-oStrictHostKeyChecking=no "
                        "-o password_stdin")
            .arg(labelTftpUserNameText())
            .arg(host)
            .arg(letter)
            .arg(labelTftpPortText());


    p->setNativeArguments(args);
    p->start();
    p->waitForStarted(50);
    if (p->state() != QProcess::Running) {
        qDebug() << "onPushButtonSftpMountDriveClicked process not started";
    }
    else {
        p->write((labelTftpPasswordText() + "\n").toUtf8());
    }

    //qDebug().noquote() << "onPushButtonSftpMountDriveClicked" << args;

#endif
}

void OtherProtocolsLogic::checkBoxSftpRestoreClicked()
{

}

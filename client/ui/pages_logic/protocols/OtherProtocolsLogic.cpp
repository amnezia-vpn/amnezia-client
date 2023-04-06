#include <QDesktopServices>
#include <QTimer>
#include <QProcess>
#include <QStorageInfo>
#include <QStandardPaths>

#include "OtherProtocolsLogic.h"
#include <functional>
#include "../../uilogic.h"
#include "utilities.h"

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif

using namespace amnezia;
using namespace PageEnumNS;

OtherProtocolsLogic::OtherProtocolsLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_checkBoxSftpRestoreChecked{false}

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
    set_pushButtonSftpMountEnabled(true);
}

#ifdef Q_OS_WINDOWS
QString OtherProtocolsLogic::getNextDriverLetter() const
{
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
        return "";
    }
    return letter;
}
#endif

//QJsonObject OtherProtocolsLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
//{

//}


void OtherProtocolsLogic::onPushButtonSftpMountDriveClicked()
{
    QString mountPath;
    QString cmd;
    QString host = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex).hostName;


#ifdef Q_OS_WINDOWS
    mountPath = getNextDriverLetter() + ":";
    //    QString cmd = QString("net use \\\\sshfs\\%1@x.x.x.x!%2 /USER:%1 %3")
    //            .arg(labelTftpUserNameText())
    //            .arg(labelTftpPortText())
    //            .arg(labelTftpPasswordText());

    cmd = "C:\\Program Files\\SSHFS-Win\\bin\\sshfs.exe";
#elif defined AMNEZIA_DESKTOP
    mountPath = QString("%1/sftp:%2:%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
            .arg(host)
            .arg(labelTftpPortText());
    QDir dir(mountPath);
    if (!dir.exists()){
      dir.mkpath(mountPath);
    }

    cmd = "/usr/local/bin/sshfs";
#endif

#ifdef AMNEZIA_DESKTOP
    set_pushButtonSftpMountEnabled(false);
    QProcess *p = new QProcess;
    m_sftpMountProcesses.append(p);
    p->setProcessChannelMode(QProcess::MergedChannels);

    connect(p, &QProcess::readyRead, this, [this, p, mountPath](){
        QString s = p->readAll();
        if (s.contains("The service sshfs has been started")) {
            QDesktopServices::openUrl(QUrl("file:///" + mountPath));
            set_pushButtonSftpMountEnabled(true);
        }
        qDebug() << s;
    });



    p->setProgram(cmd);

    QString args = QString(
                        "%1@%2:/ %3 "
                        "-o port=%4 "
                        "-f "
                        "-o reconnect "
                        "-o rellinks "
                        "-o fstypename=SSHFS "
                        "-o ssh_command=/usr/bin/ssh.exe "
                        "-o UserKnownHostsFile=/dev/null "
                        "-o StrictHostKeyChecking=no "
                        "-o password_stdin")
            .arg(labelTftpUserNameText())
            .arg(host)
            .arg(mountPath)
            .arg(labelTftpPortText());


//    args.replace("\n", " ");
//    args.replace("\r", " ");
//#ifndef Q_OS_WIN
//    args.replace("reconnect-orellinks", "");
//#endif
    p->setArguments(args.split(" ", Qt::SkipEmptyParts));
    p->start();
    p->waitForStarted(50);
    if (p->state() != QProcess::Running) {
        qDebug() << "onPushButtonSftpMountDriveClicked process not started";
        qDebug() << args;
    }
    else {
        p->write((labelTftpPasswordText() + "\n").toUtf8());
    }

    //qDebug().noquote() << "onPushButtonSftpMountDriveClicked" << args;

    set_pushButtonSftpMountEnabled(true);
#endif
}

void OtherProtocolsLogic::checkBoxSftpRestoreClicked()
{

}

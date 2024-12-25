#include "ipctun2socksprocess.h"
#include "ipc.h"
#include <QProcess>
#include <QString>

#include "../protocols/protocols_defs.h"

#ifndef Q_OS_IOS

IpcProcessTun2Socks::IpcProcessTun2Socks(QObject *parent) :
    IpcProcessTun2SocksSource(parent),
    m_t2sProcess(QSharedPointer<QProcess>(new QProcess()))
{
    qDebug() << "IpcProcessTun2Socks::IpcProcessTun2Socks()";

}

IpcProcessTun2Socks::~IpcProcessTun2Socks()
{
    qDebug() << "IpcProcessTun2Socks::~IpcProcessTun2Socks()";
}

void IpcProcessTun2Socks::start()
{
    connect(m_t2sProcess.data(), &QProcess::stateChanged, this, &IpcProcessTun2Socks::stateChanged);
    qDebug() << "IpcProcessTun2Socks::start()";
    m_t2sProcess->setProgram(amnezia::permittedProcessPath(static_cast<amnezia::PermittedProcess>(amnezia::PermittedProcess::Tun2Socks)));

    QString XrayConStr = "socks5://127.0.0.1:10808";

#ifdef Q_OS_WIN
    QStringList arguments({"-device", "tun://tun2", "-proxy", XrayConStr, "-tun-post-up",
                           QString("cmd /c netsh interface ip set address name=\"tun2\" static %1 255.255.255.255")
                               .arg(amnezia::protocols::xray::defaultLocalAddr)});
#endif
#ifdef Q_OS_LINUX
    QStringList arguments({"-device", "tun://tun2", "-proxy", XrayConStr});
#endif
#ifdef Q_OS_MAC
    QStringList arguments({"-device", "utun22", "-proxy", XrayConStr});
#endif

    m_t2sProcess->setArguments(arguments);

    if (Utils::processIsRunning(Utils::executable("tun2socks", false))) {
        qDebug().noquote() << "kill previos tun2socks";
        Utils::killProcessByName(Utils::executable("tun2socks", false));
    }

    m_t2sProcess->start();

    connect(m_t2sProcess.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        QString line = m_t2sProcess.data()->readAllStandardOutput();
        if (line.contains("[STACK] tun://") && line.contains("<-> socks5://127.0.0.1")) {
            emit setConnectionState(Vpn::ConnectionState::Connected);
        }
    });

    connect(m_t2sProcess.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "tun2socks finished, exitCode, exiStatus" << exitCode << exitStatus;
        emit setConnectionState(Vpn::ConnectionState::Disconnected);
        if ((exitStatus != QProcess::NormalExit) || (exitCode != 0)) {
            emit setConnectionState(Vpn::ConnectionState::Error);
        }

    });

    m_t2sProcess->start();
    m_t2sProcess->waitForStarted();
}

void IpcProcessTun2Socks::stop()
{
    qDebug() << "IpcProcessTun2Socks::stop()";
    m_t2sProcess->disconnect();
    m_t2sProcess->kill();
    m_t2sProcess->waitForFinished(3000);
}
#endif

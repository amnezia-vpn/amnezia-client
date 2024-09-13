#include "goodByeDpi.h"

#include "core/ipcclient.h"
#include "utilities.h"

GoodByeDpi::GoodByeDpi(QObject *parent) : QObject { parent }
{
}

amnezia::ErrorCode GoodByeDpi::start(const QString &blackListFile, const int modset)
{
    if (!QFileInfo::exists(Utils::goodbyedpiPath())) {
        return amnezia::ErrorCode::GoodByeDPIExecutableMissing;
    }

    m_goodbyeDPIProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_goodbyeDPIProcess) {
        return amnezia::ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_goodbyeDPIProcess->waitForSource(1000);
    if (!m_goodbyeDPIProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        return amnezia::ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_goodbyeDPIProcess->setProgram(amnezia::PermittedProcess::GoodbyeDPI);

    QStringList arguments;
    arguments << QString("-%1").arg(modset);
    arguments << QString("--blacklist %1").arg(blackListFile);

    m_goodbyeDPIProcess->setArguments(arguments);
    qDebug() << arguments.join(" ");

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::errorOccurred,
            [&](QProcess::ProcessError error) { qDebug() << "PrivilegedProcess errorOccurred" << error; });

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::stateChanged, [&](QProcess::ProcessState newState) {
        qDebug() << "PrivilegedProcess stateChanged" << newState;
        if (newState == QProcess::Running) {
            qDebug() << "PrivilegedProcess running";
            emit serviceStateChanged(Vpn::ConnectionState::Connected);
        }
    });

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::finished, this, [&]() {
        qDebug() << "PrivilegedProcess finished";
        emit serviceStateChanged(Vpn::ConnectionState::Disconnected);
    });

    m_goodbyeDPIProcess->start();
    return amnezia::ErrorCode::NoError;
}

void GoodByeDpi::stop()
{
    if (m_goodbyeDPIProcess) {
        m_goodbyeDPIProcess->close();
    }
}

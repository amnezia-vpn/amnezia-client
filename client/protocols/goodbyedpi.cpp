#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

#include <QThread>

#include <chrono>

#include "logger.h"
#include "goodbyedpi.h"
#include "utilities.h"


GoodbyeDPIProtocol::GoodbyeDPIProtocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    qDebug() << "GoodbyeDPIProtocol::GoodbyeDPIProtocol()";
}

GoodbyeDPIProtocol::~GoodbyeDPIProtocol()
{
    qDebug() << "GoodbyeDPIProtocol::~GoodbyeDPIProtocol()";
    GoodbyeDPIProtocol::stop();
}

void GoodbyeDPIProtocol::stop()
{
    if (m_goodbyeDPIProcess) {
        m_goodbyeDPIProcess->close();
    }
    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode GoodbyeDPIProtocol::start()
{
    qDebug() << "GoodbyeDPIProtocol::start()";

    if (!QFileInfo::exists(Utils::goodbyedpiPath())) {
        setLastError(ErrorCode::GoodByeDPIExecutableMissing);
        return lastError();
    }

    m_goodbyeDPIProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_goodbyeDPIProcess) {
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_goodbyeDPIProcess->waitForSource(1000);
    if (!m_goodbyeDPIProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_goodbyeDPIProcess->setProgram(PermittedProcess::GoodbyeDPI);

    QStringList arguments({"-9", "--blacklist", QCoreApplication::applicationDirPath() + "/goodbyedpi/russia-blacklist.txt",
                           "--blacklist", QCoreApplication::applicationDirPath() + "/goodbyedpi/russia-youtube.txt"});

    m_goodbyeDPIProcess->setArguments(arguments);
    qDebug() << arguments.join(" ");

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::errorOccurred,
            [&](QProcess::ProcessError error) { qDebug() << "PrivilegedProcess errorOccurred" << error; });

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::stateChanged,
            [&](QProcess::ProcessState newState) {
                qDebug() << "PrivilegedProcess stateChanged" << newState;
        if (newState == QProcess::Running)
        {
            setConnectionState(Vpn::ConnectionState::Connected);
        }
    });

    connect(m_goodbyeDPIProcess.data(), &PrivilegedProcess::finished, this,
        [&]() {
            setConnectionState(Vpn::ConnectionState::Disconnected);
        });


    m_goodbyeDPIProcess->start();
    return ErrorCode::NoError;
}

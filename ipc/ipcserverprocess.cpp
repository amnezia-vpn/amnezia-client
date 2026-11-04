#include "ipcserverprocess.h"
#include "ipc.h"
#include <QProcess>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <aclapi.h>
#include <sddl.h>
#endif

#ifndef Q_OS_IOS

namespace {
bool restrictToOwner(const QString &path)
{
#ifdef Q_OS_WIN
    constexpr const char *ownerOnlySddl = "D:P(A;;FA;;;SY)(A;;FA;;;BA)";

    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(ownerOnlySddl, SDDL_REVISION_1, &descriptor, nullptr)) {
        return false;
    }

    PACL dacl = nullptr;
    BOOL present = FALSE;
    BOOL defaulted = FALSE;
    bool ok = GetSecurityDescriptorDacl(descriptor, &present, &dacl, &defaulted) && present
            && SetNamedSecurityInfoW((LPWSTR)path.utf16(), SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                     dacl, nullptr)
                    == ERROR_SUCCESS;

    LocalFree(descriptor);
    return ok;
#else
    return QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
#endif
}

QString quoteYamlScalar(const QString &value)
{
    return QString(value).replace("'", "''");
}
} // namespace

IpcServerProcess::IpcServerProcess(QObject *parent) :
    IpcProcessInterfaceSource(parent),
    m_process(QSharedPointer<QProcess>(new QProcess()))
{
    connect(m_process.data(), &QProcess::errorOccurred, this, &IpcServerProcess::errorOccurred);
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &IpcServerProcess::finished);
    connect(m_process.data(), &QProcess::readyRead, this, &IpcServerProcess::readyRead);
    connect(m_process.data(), &QProcess::readyReadStandardError, this, &IpcServerProcess::readyReadStandardError);
    connect(m_process.data(), &QProcess::readyReadStandardOutput, this, &IpcServerProcess::readyReadStandardOutput);
    connect(m_process.data(), &QProcess::started, this, &IpcServerProcess::started);
    connect(m_process.data(), &QProcess::stateChanged, this, &IpcServerProcess::stateChanged);

    connect(m_process.data(), &QProcess::errorOccurred, [&](QProcess::ProcessError error){
        qDebug() << "IpcServerProcess errorOccurred " << error;
    });

    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this]() { m_tun2socksConfig.reset(); });

}

IpcServerProcess::~IpcServerProcess()
{
    qDebug() << "IpcServerProcess::~IpcServerProcess";
}

void IpcServerProcess::start()
{
    if (m_process->program().isEmpty()) {
        qDebug() << "IpcServerProcess failed to start, program is empty";
    }

    Utils::killProcessByName(m_process->program());
    m_process->start();
    qDebug() << "IpcServerProcess started, " << m_process->program() << m_process->arguments();

    m_process->waitForStarted();
}

void IpcServerProcess::terminate() {
    m_process->terminate();
}

void IpcServerProcess::kill() {
    m_process->kill();
}

void IpcServerProcess::close()
{
    m_process->close();
}

void IpcServerProcess::setArguments(const QStringList &arguments)
{
    m_process->setArguments(amnezia::sanitizeArguments(m_program, arguments));
}

void IpcServerProcess::setInputChannelMode(QProcess::InputChannelMode mode)
{
     m_process->setInputChannelMode(mode);
}

void IpcServerProcess::setNativeArguments(const QString &arguments)
{
#ifdef Q_OS_WIN
    m_process->setNativeArguments(arguments);
#endif
}

void IpcServerProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_process->setProcessChannelMode(mode);
}

void IpcServerProcess::setProgram(int programId)
{
    Q_ASSERT(programId > static_cast<int>(amnezia::PermittedProcess::Invalid) &&
             programId < static_cast<int>(amnezia::PermittedProcess::PermittedProcessCount));
    m_program = static_cast<amnezia::PermittedProcess>(programId);
    m_process->setProgram(amnezia::permittedProcessPath(m_program));
    m_process->setArguments({});
}

void IpcServerProcess::setTun2SocksConfig(const QString &device, const QString &proxy)
{
    if (m_program != amnezia::PermittedProcess::Tun2Socks) {
        qWarning() << "Refusing a tun2socks config for another program";
        return;
    }
    if (!device.startsWith("tun://") || !proxy.startsWith("socks5://")) {
        qWarning() << "Refusing a malformed tun2socks config";
        return;
    }

    m_tun2socksConfig = QSharedPointer<QTemporaryFile>(new QTemporaryFile());
    if (!m_tun2socksConfig->open()) {
        qWarning() << "Failed to create the tun2socks config file";
        m_tun2socksConfig.reset();
        return;
    }

    if (!restrictToOwner(m_tun2socksConfig->fileName())) {
        qWarning() << "Failed to restrict the tun2socks config file";
        m_tun2socksConfig.reset();
        return;
    }

    m_tun2socksConfig->write(
            QString("device: '%1'\nproxy: '%2'\n").arg(quoteYamlScalar(device), quoteYamlScalar(proxy)).toUtf8());
    m_tun2socksConfig->close();

    m_process->setArguments({ "-config", m_tun2socksConfig->fileName() });
}

void IpcServerProcess::setWorkingDirectory(const QString &dir)
{
    m_process->setWorkingDirectory(dir);
}

void IpcServerProcess::write(const QByteArray &data)
{
    m_process->write(data);
}

void IpcServerProcess::closeWriteChannel()
{
    m_process->closeWriteChannel();
}

QByteArray IpcServerProcess::readAll()
{
    return m_process->readAll();
}

QByteArray IpcServerProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

QByteArray IpcServerProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

bool IpcServerProcess::waitForStarted() {
    return m_process->waitForStarted();
}

bool IpcServerProcess::waitForStarted(int msecs) {
    return m_process->waitForStarted(msecs);
}

bool IpcServerProcess::waitForFinished() {
    return m_process->waitForFinished();
}

bool IpcServerProcess::waitForFinished(int msecs) {
    return m_process->waitForFinished(msecs);
}

#endif

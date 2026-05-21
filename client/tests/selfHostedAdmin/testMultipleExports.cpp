#include <QDebug>
#include <QUuid>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "secureQSettings.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"

using namespace amnezia;
using namespace amnezia::test;

namespace {

enum class NativeExportKind {
    Awg = 1,
    WireGuard,
    OpenVpn,
    Xray,
};

QList<DockerContainer> vpnContainersForExport()
{
    return {
        DockerContainer::Awg2,
        DockerContainer::WireGuard,
        DockerContainer::OpenVpn,
        DockerContainer::Xray
    };
}

} // namespace

class TestMultipleExports : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;
    QString m_serverId;

    ServerCredentials getCredentialsFromIni()
    {
        ServerCredentials credentials;
        credentials.hostName = getValueFromIni("secrets/selfHostedServerHostName");
        credentials.userName = getValueFromIni("secrets/selfHostedServerUserName");
        credentials.secretData = getValueFromIni("secrets/selfHostedServerPassword");
        credentials.port = getValueFromIni("secrets/selfHostedServerSshPort").toInt();
        return credentials;
    }

    int portForContainer(DockerContainer container) const
    {
        const Proto proto = ContainerUtils::defaultProtocol(container);
        if (container == DockerContainer::Awg2) {
            return ProtocolUtils::getPortForInstall(proto);
        }
        return ProtocolUtils::defaultPort(proto);
    }

    TransportProto transportForContainer(DockerContainer container) const
    {
        return ProtocolUtils::defaultTransportProto(ContainerUtils::defaultProtocol(container));
    }

    void installVpnContainer(DockerContainer container)
    {
        const QString containerName = ContainerUtils::containerToString(container);
        bool wasInstalled = false;
        const ErrorCode error = m_coreController->m_installController->installContainer(
            m_serverId, container, portForContainer(container), transportForContainer(container), wasInstalled);

        QVERIFY2(error == ErrorCode::NoError,
                 QString("%1: installContainer should succeed. Error: %2")
                     .arg(containerName)
                     .arg(static_cast<int>(error))
                     .toUtf8()
                     .constData());
        qDebug() << containerName << "installed:" << wasInstalled;
    }

    void setupServerWithVpnContainers()
    {
        const ServerCredentials credentials = getCredentialsFromIni();

        if (credentials.hostName.isEmpty() || credentials.userName.isEmpty() || credentials.secretData.isEmpty()) {
            QSKIP("Test requires selfHostedServerHostName, selfHostedServerUserName, selfHostedServerPassword in test_vars.ini");
        }

        QVERIFY2(credentials.isValid(), "Server credentials should be valid");
        qDebug() << "Using server:" << credentials.hostName << "user:" << credentials.userName
                 << "port:" << credentials.port;

        QString sshOutput;
        const ErrorCode sshError =
            m_coreController->m_installController->checkSshConnection(credentials, sshOutput);
        QVERIFY2(sshError == ErrorCode::NoError,
                 QString("SSH connection should succeed. Error: %1, Output: %2")
                     .arg(static_cast<int>(sshError))
                     .arg(sshOutput)
                     .toUtf8()
                     .constData());

        m_coreController->m_installController->addEmptyServer(credentials);
        QVERIFY2(m_coreController->m_serversRepository->serversCount() > 0, "Server should be added");

        m_serverId = m_coreController->m_serversRepository->defaultServerId();
        QVERIFY2(!m_serverId.isEmpty(), "Server id should not be empty");

        const ErrorCode removeError = m_coreController->m_installController->removeAllContainers(m_serverId);
        QVERIFY2(removeError == ErrorCode::NoError,
                 QString("removeAllContainers should succeed. Error: %1")
                     .arg(static_cast<int>(removeError))
                     .toUtf8()
                     .constData());
        qDebug() << "Remote Amnezia services cleared";

        for (DockerContainer container : vpnContainersForExport()) {
            installVpnContainer(container);
        }

        qDebug() << "All VPN containers installed for export tests";
    }

    void skipIfContainerNotInstalled(const QString &containerName, int containerIndex)
    {
        if (!m_coreController->m_containersModel->data(containerIndex, ContainersModel::Roles::IsInstalledRole).toBool()) {
            const QString reason = QString("%1: Not installed").arg(containerName);
            QSKIP(reason.toUtf8().constData());
        }
    }

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);

        setupServerWithVpnContainers();
    }

    void cleanupTestCase()
    {
        if (!m_serverId.isEmpty()) {
            m_coreController->m_installController->removeAllContainers(m_serverId);

            for (int containerIndex = 1; containerIndex < 7; ++containerIndex) {
                m_coreController->m_installUiController->clearCachedProfile(m_serverId, containerIndex);
            }

            m_coreController->m_serversController->removeServer(m_serverId);

            qDebug() << "SERVER REMOVED\n";
        }

        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void testMultipleExports_data()
    {
        QTest::addColumn<QString>("containerName");
        QTest::addColumn<int>("containerIndex");

        QTest::newRow("awg2") << "Awg2" << static_cast<int>(DockerContainer::Awg2);
        QTest::newRow("wireguard") << "WireGuard" << static_cast<int>(DockerContainer::WireGuard);
        QTest::newRow("openvpn") << "OpenVPN" << static_cast<int>(DockerContainer::OpenVpn);
        QTest::newRow("xray") << "XRay" << static_cast<int>(DockerContainer::Xray);
    }

    void testMultipleExports()
    {
        QFETCH(QString, containerName);
        QFETCH(int, containerIndex);

        skipIfContainerNotInstalled(containerName, containerIndex);

        const QString clientName = "MultipleExports Test Client";
        const auto exportResult =
            m_coreController->m_exportController->generateConnectionConfig(m_serverId, containerIndex, clientName);

        QVERIFY2(exportResult.errorCode == ErrorCode::NoError,
                 QString("\n%1: Export should succeed").arg(containerName).toUtf8().constData());
        QVERIFY2(!exportResult.config.isEmpty(),
                 QString("%1: Exported config should not be empty").arg(containerName).toUtf8().constData());
    }

    void testMultipleExportsNative_data()
    {
        QTest::addColumn<QString>("containerName");
        QTest::addColumn<int>("containerIndex");
        QTest::addColumn<int>("nativeExportKind");

        QTest::newRow("awg") << "Awg2" << static_cast<int>(DockerContainer::Awg2)
                             << static_cast<int>(NativeExportKind::Awg);
        QTest::newRow("wireguard") << "WireGuard" << static_cast<int>(DockerContainer::WireGuard)
                                   << static_cast<int>(NativeExportKind::WireGuard);
        QTest::newRow("openvpn") << "OpenVPN" << static_cast<int>(DockerContainer::OpenVpn)
                                 << static_cast<int>(NativeExportKind::OpenVpn);
        QTest::newRow("xray") << "XRay" << static_cast<int>(DockerContainer::Xray)
                              << static_cast<int>(NativeExportKind::Xray);
    }

    void testMultipleExportsNative()
    {
        QFETCH(QString, containerName);
        QFETCH(int, containerIndex);
        QFETCH(int, nativeExportKind);

        skipIfContainerNotInstalled(containerName, containerIndex);

        const QString clientName = "MultipleExports Test Client";
        ExportController::ExportResult exportResult;

        switch (static_cast<NativeExportKind>(nativeExportKind)) {
        case NativeExportKind::Awg:
            exportResult = m_coreController->m_exportController->generateAwgConfig(m_serverId, containerIndex, clientName);
            break;
        case NativeExportKind::WireGuard:
            exportResult = m_coreController->m_exportController->generateWireGuardConfig(m_serverId, clientName);
            break;
        case NativeExportKind::OpenVpn:
            exportResult = m_coreController->m_exportController->generateOpenVpnConfig(m_serverId, clientName);
            break;
        case NativeExportKind::Xray:
            exportResult = m_coreController->m_exportController->generateXrayConfig(m_serverId, clientName);
            break;
        }

        QVERIFY2(exportResult.errorCode == ErrorCode::NoError,
                 QString("\n%1 (native): Export should succeed").arg(containerName).toUtf8().constData());
        QVERIFY2(!exportResult.config.isEmpty(),
                 QString("%1 (native): Exported config should not be empty").arg(containerName).toUtf8().constData());
    }
};

QTEST_MAIN(TestMultipleExports)
#include "testMultipleExports.moc"

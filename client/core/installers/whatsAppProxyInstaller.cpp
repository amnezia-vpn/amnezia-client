#include "whatsAppProxyInstaller.h"

#include "core/models/containerConfig.h"
#include "core/utils/containerEnum.h"
#include "core/utils/selfhosted/sshSession.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

using namespace amnezia;

namespace
{
    constexpr QLatin1String kClientJsonPath("/data/amnezia-whatsappproxy-client.json");
    constexpr QLatin1String kClientJsonUploadPath("data/amnezia-whatsappproxy-client.json");
}

void WhatsAppProxyInstaller::applyDockerPublishedPorts(const QString &dockerPsPortsLine,
                                                       WhatsAppProxyProtocolConfig &config)
{
    static const QRegularExpression chatRe(QStringLiteral(":(\\d+)->443/tcp"));
    static const QRegularExpression mediaRe(QStringLiteral(":(\\d+)->587/tcp"));
    const QRegularExpressionMatch chatMatch = chatRe.match(dockerPsPortsLine);
    const QRegularExpressionMatch mediaMatch = mediaRe.match(dockerPsPortsLine);
    if (chatMatch.hasMatch()) {
        config.port = chatMatch.captured(1);
    }
    if (mediaMatch.hasMatch()) {
        config.mediaPort = mediaMatch.captured(1);
    }
}

WhatsAppProxyInstaller::WhatsAppProxyInstaller(QObject *parent) : InstallerBase(parent)
{
}

ErrorCode WhatsAppProxyInstaller::extractConfigFromContainer(DockerContainer container,
                                                             const ServerCredentials &credentials,
                                                             SshSession *sshSession, ContainerConfig &config)
{
    if (container != DockerContainer::WhatsAppProxy || !sshSession) {
        return ErrorCode::NoError;
    }

    WhatsAppProxyProtocolConfig *wc = config.getWhatsAppProxyProtocolConfig();
    if (!wc) {
        return ErrorCode::NoError;
    }

    ErrorCode jsonErr = ErrorCode::NoError;
    const QByteArray jsonRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kClientJsonPath), jsonErr);
    if (jsonErr != ErrorCode::NoError || jsonRaw.trimmed().isEmpty()) {
        return ErrorCode::NoError;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonRaw.trimmed(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return ErrorCode::NoError;
    }

    const WhatsAppProxyProtocolConfig snapshot = WhatsAppProxyProtocolConfig::fromJson(doc.object());
    wc->publicHost = snapshot.publicHost;
    wc->isEnabled = snapshot.isEnabled;
    return ErrorCode::NoError;
}

void WhatsAppProxyInstaller::uploadClientSettingsSnapshot(SshSession &sshSession, const ServerCredentials &credentials,
                                                          DockerContainer container, const ContainerConfig &config)
{
    const WhatsAppProxyProtocolConfig *wc = config.getWhatsAppProxyProtocolConfig();
    if (!wc) {
        return;
    }
    const QByteArray payload = QJsonDocument(wc->toJson()).toJson(QJsonDocument::Compact);
    const ErrorCode err = sshSession.uploadTextFileToContainer(container, credentials, QString::fromUtf8(payload),
                                                               QString(kClientJsonUploadPath));
    if (err != ErrorCode::NoError) {
        qWarning() << "WhatsAppProxyInstaller::uploadClientSettingsSnapshot failed" << err;
    }
}

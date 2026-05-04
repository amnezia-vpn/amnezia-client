#include "mtProxyInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

#include <QtGlobal>

using namespace amnezia;

namespace {
    constexpr QLatin1String kMtProxyClientJsonPath("/data/amnezia-mtproxy-client.json");
    constexpr QLatin1String kMtProxyClientJsonUploadPath("data/amnezia-mtproxy-client.json");
    constexpr QLatin1String kMtProxySecretPath("/data/secret");
}

MtProxyInstaller::MtProxyInstaller(QObject *parent)
        : InstallerBase(parent) {
}

ErrorCode MtProxyInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                       SshSession *sshSession, ContainerConfig &config) {
    if (container != DockerContainer::MtProxy || !sshSession) {
        return ErrorCode::NoError;
    }

    MtProxyProtocolConfig *mt = config.getMtProxyProtocolConfig();
    if (!mt) {
        return ErrorCode::NoError;
    }

    ErrorCode jsonErr = ErrorCode::NoError;
    const QByteArray jsonRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kMtProxyClientJsonPath), jsonErr);
    if (jsonErr == ErrorCode::NoError && !jsonRaw.trimmed().isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonRaw.trimmed(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject merged = mt->toJson();
            const QJsonObject snap = doc.object();
            for (auto it = snap.constBegin(); it != snap.constEnd(); ++it) {
                merged.insert(it.key(), it.value());
            }
            *mt = MtProxyProtocolConfig::fromJson(merged);
        }
    }

    ErrorCode secretErr = ErrorCode::NoError;
    const QByteArray secretRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kMtProxySecretPath), secretErr);
    const QString sec = QString::fromUtf8(secretRaw).trimmed();
    if (sec.length() == 32) {
        static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
        if (hex32.match(sec).hasMatch()) {
            mt->secret = sec;
        }
    }

    return ErrorCode::NoError;
}

void MtProxyInstaller::uploadClientSettingsSnapshot(SshSession &sshSession, const ServerCredentials &credentials,
                                                    DockerContainer container, const ContainerConfig &config) {
    const MtProxyProtocolConfig *mt = config.getMtProxyProtocolConfig();
    if (!mt) {
        return;
    }
    const QByteArray payload = QJsonDocument(mt->toJson()).toJson(QJsonDocument::Compact);
    const ErrorCode err = sshSession.uploadTextFileToContainer(container, credentials, QString::fromUtf8(payload),
                                                               QString(kMtProxyClientJsonUploadPath));
    if (err != ErrorCode::NoError) {
        qWarning() << "MtProxyInstaller::uploadClientSettingsSnapshot failed" << err;
    }
}

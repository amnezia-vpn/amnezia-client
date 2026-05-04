#include "telemtInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/telemtProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

#include <QtGlobal>

using namespace amnezia;

namespace {
    constexpr QLatin1String kTelemtClientJsonPath("/data/amnezia-telemt-client.json");
    constexpr QLatin1String kTelemtClientJsonUploadPath("data/amnezia-telemt-client.json");
    constexpr QLatin1String kTelemtSecretPath("/data/secret");
}

TelemtInstaller::TelemtInstaller(QObject *parent) : InstallerBase(parent) {}

ErrorCode TelemtInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                      SshSession *sshSession, ContainerConfig &config) {
    if (container != DockerContainer::Telemt || !sshSession) {
        return ErrorCode::NoError;
    }

    TelemtProtocolConfig *tc = config.getTelemtProtocolConfig();
    if (!tc) {
        return ErrorCode::NoError;
    }

    ErrorCode jsonErr = ErrorCode::NoError;
    const QByteArray jsonRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTelemtClientJsonPath), jsonErr);
    if (jsonErr == ErrorCode::NoError && !jsonRaw.trimmed().isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonRaw.trimmed(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject merged = tc->toJson();
            const QJsonObject snap = doc.object();
            for (auto it = snap.constBegin(); it != snap.constEnd(); ++it) {
                merged.insert(it.key(), it.value());
            }
            *tc = TelemtProtocolConfig::fromJson(merged);
        }
    }

    ErrorCode secretErr = ErrorCode::NoError;
    const QByteArray secretRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTelemtSecretPath), secretErr);
    const QString sec = QString::fromUtf8(secretRaw).trimmed();
    if (sec.length() == 32) {
        static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
        if (hex32.match(sec).hasMatch()) {
            tc->secret = sec;
        }
    }

    return ErrorCode::NoError;
}

void TelemtInstaller::uploadClientSettingsSnapshot(SshSession &sshSession, const ServerCredentials &credentials,
                                                   DockerContainer container, const ContainerConfig &config) {
    const TelemtProtocolConfig *tc = config.getTelemtProtocolConfig();
    if (!tc) {
        return;
    }
    const QByteArray payload = QJsonDocument(tc->toJson()).toJson(QJsonDocument::Compact);
    const ErrorCode err = sshSession.uploadTextFileToContainer(container, credentials, QString::fromUtf8(payload),
                                                               QString(kTelemtClientJsonUploadPath));
    if (err != ErrorCode::NoError) {
        qWarning() << "TelemtInstaller::uploadClientSettingsSnapshot failed" << err;
    }
}

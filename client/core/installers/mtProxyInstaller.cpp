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

ErrorCode MtProxyInstaller::queryDiagnostics(SshSession &sshSession, const ServerCredentials &credentials,
                                              DockerContainer container, int listenPort,
                                              MtProxyContainerDiagnostics &out)
{
    out = {};
    if (container != DockerContainer::MtProxy) {
        return ErrorCode::InternalError;
    }
    const QString containerName = ContainerUtils::containerToString(container);
    const QString script =
            QStringLiteral(
                    "PORT_OK=$(sudo docker exec %1 sh -c 'ss -tlnp 2>/dev/null | grep -q :%2 && echo yes || echo no' 2>/dev/null || echo no); "
                    "TG_OK=$(curl -s --max-time 5 -o /dev/null -w '%%{http_code}' https://core.telegram.org/getProxySecret 2>/dev/null | grep -q '200' && echo yes || echo no); "
                    "CLIENTS=$(sudo docker exec amnezia-mtproxy sh -c 'curl -s --max-time 3 http://localhost:2398/stats 2>/dev/null | grep -o \"total_special_connections:[0-9]*\" | cut -d: -f2' 2>/dev/null); "
                    "CONF_TIME=$(sudo docker exec amnezia-mtproxy sh -c 'stat -c \"%%y\" /data/proxy-multi.conf 2>/dev/null | cut -d. -f1' 2>/dev/null || echo unknown); "
                    "echo \"PORT_OK=${PORT_OK}\"; "
                    "echo \"TG_OK=${TG_OK}\"; "
                    "echo \"CLIENTS=${CLIENTS:-0}\"; "
                    "echo \"CONF_TIME=${CONF_TIME}\"; "
                    "echo \"STATS=http://localhost:2398/stats\";")
                    .arg(containerName)
                    .arg(listenPort);

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };
    const ErrorCode errorCode = sshSession.runScript(credentials, script, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    for (const QString &line : stdOut.split('\n', Qt::SkipEmptyParts)) {
        if (line.startsWith(QLatin1String("PORT_OK="))) {
            out.portReachable = line.mid(8).trimmed() == QLatin1String("yes");
        } else if (line.startsWith(QLatin1String("TG_OK="))) {
            out.upstreamReachable = line.mid(6).trimmed() == QLatin1String("yes");
        } else if (line.startsWith(QLatin1String("CLIENTS="))) {
            out.clientsConnected = line.mid(8).trimmed().toInt();
        } else if (line.startsWith(QLatin1String("CONF_TIME="))) {
            out.lastConfigRefresh = line.mid(10).trimmed();
        } else if (line.startsWith(QLatin1String("STATS="))) {
            out.statsEndpoint = line.mid(6).trimmed();
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

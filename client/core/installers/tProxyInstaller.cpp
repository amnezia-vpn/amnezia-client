#include "tProxyInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/tProxyProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

#include <QtGlobal>

using namespace amnezia;

namespace {
    constexpr QLatin1String kTProxyClientJsonPath("/data/amnezia-tproxy-client.json");
    constexpr QLatin1String kTProxyClientJsonUploadPath("data/amnezia-tproxy-client.json");
    constexpr QLatin1String kTProxySecretPath("/data/secret");
    constexpr QLatin1String kTProxyMetaPath("/data/tproxy-meta");

    void rebuildTProxyLinksIfNeeded(TProxyProtocolConfig *tc)
    {
        if (!tc || tc->hostname.isEmpty() || tc->secret.isEmpty()) {
            return;
        }
        const QString host = tc->hostname;
        const QString sec = tc->secret;
        // WEB proxy type fixes HTTPS/443; the canonical webproxy link carries no port.
        if (tc->tgLink.isEmpty()) {
            tc->tgLink = QStringLiteral("tg://webproxy?server=%1&secret=%2").arg(host, sec);
        }
        if (tc->tmeLink.isEmpty()) {
            tc->tmeLink = QStringLiteral("https://t.me/webproxy?server=%1&secret=%2").arg(host, sec);
        }
    }
}

void TProxyInstaller::applyDockerPublishedPorts(const QString &dockerPsPortsLine, TProxyProtocolConfig &config)
{
    static const QRegularExpression httpRe(QStringLiteral(":(\\d+)->80/(tcp|udp)"));
    static const QRegularExpression httpsRe(QStringLiteral(":(\\d+)->443/(tcp|udp)"));
    const QRegularExpressionMatch httpMatch = httpRe.match(dockerPsPortsLine);
    const QRegularExpressionMatch httpsMatch = httpsRe.match(dockerPsPortsLine);
    if (httpsMatch.hasMatch()) {
        config.port = httpsMatch.captured(1);
    }
    if (httpMatch.hasMatch()) {
        config.httpPort = httpMatch.captured(1);
    }
}

TProxyInstaller::TProxyInstaller(QObject *parent) : InstallerBase(parent) {}

ErrorCode TProxyInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                      SshSession *sshSession, ContainerConfig &config)
{
    if (container != DockerContainer::TProxy || !sshSession) {
        return ErrorCode::NoError;
    }

    TProxyProtocolConfig *tc = config.getTProxyProtocolConfig();
    if (!tc) {
        return ErrorCode::NoError;
    }

    ErrorCode jsonErr = ErrorCode::NoError;
    const QByteArray jsonRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTProxyClientJsonPath), jsonErr);
    if (jsonErr == ErrorCode::NoError && !jsonRaw.trimmed().isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonRaw.trimmed(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject merged = tc->toJson();
            const QJsonObject snap = doc.object();
            for (auto it = snap.constBegin(); it != snap.constEnd(); ++it) {
                merged.insert(it.key(), it.value());
            }
            *tc = TProxyProtocolConfig::fromJson(merged);
        }
    }

    static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
    ErrorCode secretErr = ErrorCode::NoError;
    const QByteArray secretRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTProxySecretPath), secretErr);
    const QString sec = QString::fromUtf8(secretRaw).trimmed();
    if (sec.length() == 32 && hex32.match(sec).hasMatch()) {
        tc->secret = sec;
    }

    ErrorCode metaErr = ErrorCode::NoError;
    const QByteArray metaRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTProxyMetaPath), metaErr);
    if (metaErr == ErrorCode::NoError && !metaRaw.trimmed().isEmpty()) {
        const QList<QByteArray> lines = metaRaw.split('\n');
        for (const QByteArray &rawLine : lines) {
            const QString line = QString::fromUtf8(rawLine).trimmed();
            const int eq = line.indexOf('=');
            if (eq < 0) {
                continue;
            }
            const QString key = line.left(eq);
            const QString val = line.mid(eq + 1).trimmed();
            if (key == QLatin1String("hostname") && tc->hostname.isEmpty()) {
                tc->hostname = val;
            } else if (key == QLatin1String("email") && tc->acmeEmail.isEmpty()) {
                tc->acmeEmail = val;
            } else if (key == QLatin1String("carrier") && tc->carrierMode.isEmpty()) {
                tc->carrierMode = val;
            } else if (key == QLatin1String("workers") && tc->workers.isEmpty()) {
                tc->workers = val;
            } else if (key == QLatin1String("http_port") && !val.isEmpty()) {
                tc->httpPort = val;
            } else if (key == QLatin1String("https_port") && !val.isEmpty()) {
                tc->port = val;
            }
        }
    }

    rebuildTProxyLinksIfNeeded(tc);

    return ErrorCode::NoError;
}

void TProxyInstaller::uploadClientSettingsSnapshot(SshSession &sshSession, const ServerCredentials &credentials,
                                                   DockerContainer container, const ContainerConfig &config)
{
    const TProxyProtocolConfig *tc = config.getTProxyProtocolConfig();
    if (!tc) {
        return;
    }
    const QByteArray payload = QJsonDocument(tc->toJson()).toJson(QJsonDocument::Compact);
    const ErrorCode err = sshSession.uploadTextFileToContainer(container, credentials, QString::fromUtf8(payload),
                                                               QString(kTProxyClientJsonUploadPath));
    if (err != ErrorCode::NoError) {
        qWarning() << "TProxyInstaller::uploadClientSettingsSnapshot failed" << err;
    }
}

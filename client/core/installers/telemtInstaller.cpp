#include "telemtInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/constants/protocolConstants.h"
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
    constexpr QLatin1String kTelemtConfigTomlPath("/data/config.toml");
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
    static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
    if (sec.length() == 32 && hex32.match(sec).hasMatch()) {
        tc->secret = sec;
    }

    ErrorCode tomlErr = ErrorCode::NoError;
    const QByteArray tomlRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kTelemtConfigTomlPath), tomlErr);
    if (tomlErr == ErrorCode::NoError && !tomlRaw.trimmed().isEmpty()) {
        QString section;
        bool userNameSet = false;
        const QList<QByteArray> lines = tomlRaw.split('\n');
        for (const QByteArray &rawLine : lines) {
            const QString line = QString::fromUtf8(rawLine).trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }
            if (line.startsWith('[')) {
                section = line;
                continue;
            }
            const int eq = line.indexOf('=');
            if (eq < 0) {
                continue;
            }
            const QString key = line.left(eq).trimmed();
            QString val = line.mid(eq + 1).trimmed();
            if (val.startsWith('"')) {
                const int last = val.lastIndexOf('"');
                val = (last > 0) ? val.mid(1, last - 1) : val.mid(1);
            } else {
                const int inlineComment = val.indexOf(QLatin1String(" #"));
                if (inlineComment >= 0) {
                    val = val.left(inlineComment).trimmed();
                }
            }

            if (section == QLatin1String("[access.users]")) {
                if (key.startsWith(QLatin1String("extra"))) {
                    if (hex32.match(val).hasMatch() && !tc->additionalSecrets.contains(val)) {
                        tc->additionalSecrets.append(val);
                    }
                } else if (!userNameSet) {
                    tc->userName = key;
                    userNameSet = true;
                    if (tc->secret.isEmpty() && hex32.match(val).hasMatch()) {
                        tc->secret = val;
                    }
                }
                continue;
            }

            if (key == QLatin1String("tls") && section == QLatin1String("[general.modes]")) {
                tc->transportMode = (val == QLatin1String("true"))
                        ? QString::fromUtf8(protocols::telemt::transportModeFakeTLS)
                        : QString::fromUtf8(protocols::telemt::transportModeStandard);
            } else if (key == QLatin1String("tls_domain") && section == QLatin1String("[censorship]")) {
                tc->tlsDomain = val;
            } else if (key == QLatin1String("mask") && section == QLatin1String("[censorship]")) {
                tc->maskEnabled = (val == QLatin1String("true"));
            } else if (key == QLatin1String("tls_emulation") && section == QLatin1String("[censorship]")) {
                tc->tlsEmulation = (val == QLatin1String("true"));
            } else if (key == QLatin1String("use_middle_proxy") && section == QLatin1String("[general]")) {
                tc->useMiddleProxy = (val == QLatin1String("true"));
            } else if (key == QLatin1String("middle_proxy_nat_ip") && section == QLatin1String("[general]")) {
                if (!val.isEmpty()) {
                    tc->natExternalIp = val;
                    tc->natEnabled = true;
                }
            } else if (key == QLatin1String("ad_tag") && section == QLatin1String("[general]") && tc->tag.isEmpty()) {
                tc->tag = val;
            } else if (key == QLatin1String("public_host") && section == QLatin1String("[general.links]")
                       && tc->publicHost.isEmpty()) {
                tc->publicHost = val;
            } else if (key == QLatin1String("port") && section == QLatin1String("[server]") && tc->port.isEmpty()) {
                tc->port = val;
            }
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

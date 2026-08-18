#include "mtProxyInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/constants/protocolConstants.h"
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
    constexpr QLatin1String kMtProxyMetaPath("/data/mtproxy-meta");
    constexpr QLatin1String kMtProxyStartScriptPath("/opt/amnezia/start.sh");
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

    static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
    const auto addExtra = [&](const QString &s) {
        if (hex32.match(s).hasMatch() && !mt->additionalSecrets.contains(s)) {
            mt->additionalSecrets.append(s);
        }
    };

    ErrorCode secretErr = ErrorCode::NoError;
    const QByteArray secretRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kMtProxySecretPath), secretErr);
    const QString sec = QString::fromUtf8(secretRaw).trimmed();
    if (sec.length() == 32 && hex32.match(sec).hasMatch()) {
        mt->secret = sec;
    }

    bool metaRestored = false;
    ErrorCode metaErr = ErrorCode::NoError;
    const QByteArray metaRaw =
            sshSession->getTextFileFromContainer(container, credentials, QString(kMtProxyMetaPath), metaErr);
    if (metaErr == ErrorCode::NoError && !metaRaw.trimmed().isEmpty()) {
        QString mode, domain, workersMode, workers, natInternal, natExternal;
        bool natEnabled = false;
        const QList<QByteArray> lines = metaRaw.split('\n');
        for (const QByteArray &rawLine : lines) {
            const QString line = QString::fromUtf8(rawLine).trimmed();
            const int eq = line.indexOf('=');
            if (eq < 0) {
                continue;
            }
            const QString key = line.left(eq);
            const QString val = line.mid(eq + 1).trimmed();
            if (key == QLatin1String("mode")) mode = val;
            else if (key == QLatin1String("domain")) domain = val;
            else if (key == QLatin1String("tag")) { if (mt->tag.isEmpty()) mt->tag = val; }
            else if (key == QLatin1String("additional")) {
                for (const QString &s : val.split(',', Qt::SkipEmptyParts)) addExtra(s.trimmed());
            }
            else if (key == QLatin1String("workers_mode")) workersMode = val;
            else if (key == QLatin1String("workers")) workers = val;
            else if (key == QLatin1String("nat_enabled")) natEnabled = (val == QLatin1String("1"));
            else if (key == QLatin1String("nat_internal")) natInternal = val;
            else if (key == QLatin1String("nat_external")) natExternal = val;
            else if (key == QLatin1String("public_host")) { if (mt->publicHost.isEmpty()) mt->publicHost = val; }
        }
        if (!mode.isEmpty()) {
            mt->transportMode = mode;
            if (!domain.isEmpty()) mt->tlsDomain = domain;
            if (!workersMode.isEmpty()) mt->workersMode = workersMode;
            if (workersMode == QLatin1String(protocols::mtProxy::workersModeManual) && !workers.isEmpty()) {
                mt->workers = workers;
            }
            if (natEnabled) {
                mt->natEnabled = true;
                mt->natInternalIp = natInternal;
                mt->natExternalIp = natExternal;
            }
            metaRestored = true;
        }
    }
    if (!metaRestored) {
        ErrorCode startErr = ErrorCode::NoError;
        const QByteArray startRaw =
                sshSession->getTextFileFromContainer(container, credentials, QString(kMtProxyStartScriptPath), startErr);
        if (startErr == ErrorCode::NoError && !startRaw.trimmed().isEmpty()) {
            const QString start = QString::fromUtf8(startRaw);
            static const QRegularExpression modeRe(QStringLiteral("\\[ \"(standard|faketls)\" = \"faketls\" \\]"));
            const QRegularExpressionMatch m = modeRe.match(start);
            if (m.hasMatch()) {
                mt->transportMode = m.captured(1);
                if (m.captured(1) == QLatin1String(protocols::mtProxy::transportModeFakeTLS)) {
                    static const QRegularExpression domRe(QStringLiteral("--domain ([A-Za-z0-9.\\-]+)"));
                    const QRegularExpressionMatch dm = domRe.match(start);
                    if (dm.hasMatch()) mt->tlsDomain = dm.captured(1);
                }
            }
            static const QRegularExpression tagRe(QStringLiteral("-P ([0-9a-fA-F]{32})"));
            const QRegularExpressionMatch tm = tagRe.match(start);
            if (tm.hasMatch() && mt->tag.isEmpty()) mt->tag = tm.captured(1);

            static const QRegularExpression addRe(QStringLiteral("echo \"([0-9a-fA-F,]+)\" \\| tr ',' ' '"));
            const QRegularExpressionMatch am = addRe.match(start);
            if (am.hasMatch()) {
                for (const QString &s : am.captured(1).split(',', Qt::SkipEmptyParts)) addExtra(s.trimmed());
            }

            static const QRegularExpression natRe(QStringLiteral("NAT_VALUE=\"([0-9.]+):([0-9.]+)\""));
            const QRegularExpressionMatch nm = natRe.match(start);
            if (nm.hasMatch()) {
                mt->natEnabled = true;
                mt->natInternalIp = nm.captured(1);
                mt->natExternalIp = nm.captured(2);
            }
        }
    }

    return ErrorCode::NoError;
}

ErrorCode MtProxyInstaller::queryDiagnostics(SshSession &sshSession, const ServerCredentials &credentials,
                                             DockerContainer container, int listenPort,
                                             MtProxyContainerDiagnostics &out)
{
    out = { };
    if (container == DockerContainer::MtProxy || container == DockerContainer::Telemt) {
        const QString containerName = ContainerUtils::containerToString(container);
        const bool isTelemt = container == DockerContainer::Telemt;

        const QString sportFilter = QString::number(listenPort);
        const QString peersCmd = QStringLiteral("sudo conntrack -L -p tcp --dport ") + sportFilter
                + QStringLiteral(" 2>/dev/null | grep ESTABLISHED | awk '{for(i=1;i<=NF;i++) if($i ~ /^src=/){print "
                                 "substr($i,5); break}}'");
        const QString publicFilter = QStringLiteral(" | grep -vE "
                                                    "'^(10\\.|127\\.|169\\.254\\.|192\\.168\\.|172\\.(1[6-9]|2[0-9]|3["
                                                    "01])\\.|::1$|fe80:|f[cd][0-9a-f][0-9a-f]:)'");
        const QString clientsCmd =
                QStringLiteral("CLIENTS=$(") + peersCmd + publicFilter + QStringLiteral(" | sort -u | grep -c .); ");
        const QString confFile =
                isTelemt ? QStringLiteral("/data/config.toml") : QStringLiteral("/data/proxy-multi.conf");
        const QString statsUrl = QString();

        const QString script = QStringLiteral("CN=") + containerName + QStringLiteral("; ")
                + QStringLiteral("PORT_OK=$(sudo ss -tlnp 2>/dev/null | grep -q :") + QString::number(listenPort)
                + QStringLiteral(" && echo yes || echo no); ")
                + QStringLiteral("TG_OK=$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "
                                 "https://core.telegram.org/getProxySecret 2>/dev/null | grep -q '200' && echo yes || "
                                 "echo no); ")
                + clientsCmd + QStringLiteral("CONF_TIME=$(sudo docker exec \"$CN\" sh -c 'stat -c \"%y\" ") + confFile
                + QStringLiteral(" 2>/dev/null | cut -d. -f1' 2>/dev/null || echo unknown); ")
                + QStringLiteral("echo \"PORT_OK=${PORT_OK}\"; ") + QStringLiteral("echo \"TG_OK=${TG_OK}\"; ")
                + QStringLiteral("echo \"CLIENTS=${CLIENTS:-0}\"; ") + QStringLiteral("echo \"CONF_TIME=${CONF_TIME}\"; ")
                + QStringLiteral("echo \"STATS=") + statsUrl + QStringLiteral("\";");

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

    return ErrorCode::InternalError;
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

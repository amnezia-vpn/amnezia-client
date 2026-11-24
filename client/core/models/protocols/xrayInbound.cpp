#include "xrayInbound.h"

#include <QJsonDocument>

#include "logger.h"

#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/constants/protocolConstants.h"

namespace
{
    Logger logger("XrayInbound");

    namespace px = amnezia::protocols::xray;

    bool isAuto(const QJsonValue &value)
    {
        return value.toString() == QLatin1String(amnezia::xrayInbound::autoValue);
    }

    QJsonArray replaceAutoShortIds(const QJsonArray &shortIds, const QString &shortId)
    {
        QJsonArray result;
        for (const QJsonValue &value : shortIds) {
            result.append(isAuto(value) ? QJsonValue(shortId) : value);
        }
        return result;
    }

    QJsonArray replaceAutoCertificates(const QJsonArray &certificates)
    {
        QJsonArray result;
        for (const QJsonValue &value : certificates) {
            QJsonObject certificate = value.toObject();
            if (isAuto(certificate.value(px::certificateFile))) {
                certificate[px::certificateFile] = QString::fromLatin1(px::tlsCertPath);
            }
            if (isAuto(certificate.value(px::keyFile))) {
                certificate[px::keyFile] = QString::fromLatin1(px::tlsKeyPath);
            }
            result.append(certificate);
        }
        return result;
    }

    QJsonArray certificatesToTemplate(const QJsonArray &certificates)
    {
        QJsonArray result;
        for (const QJsonValue &value : certificates) {
            QJsonObject certificate = value.toObject();
            if (certificate.value(px::certificateFile).toString() == QLatin1String(px::tlsCertPath)) {
                certificate[px::certificateFile] = QString::fromLatin1(amnezia::xrayInbound::autoValue);
            }
            if (certificate.value(px::keyFile).toString() == QLatin1String(px::tlsKeyPath)) {
                certificate[px::keyFile] = QString::fromLatin1(amnezia::xrayInbound::autoValue);
            }
            result.append(certificate);
        }
        return result;
    }
}

namespace amnezia::xrayInbound
{

    QJsonObject parse(const QString &templateText)
    {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(templateText.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            logger.warning() << "parse: not a JSON object, error" << error.errorString() << "offset"
                             << QString::number(error.offset) << "length" << QString::number(templateText.size());
            return {};
        }
        return document.object();
    }

    QString format(const QJsonObject &inbound)
    {
        return QString::fromUtf8(QJsonDocument(inbound).toJson(QJsonDocument::Indented)).trimmed();
    }

    QString normalized(const QString &templateText)
    {
        const QJsonObject inbound = parse(templateText);
        if (inbound.isEmpty()) {
            return templateText.trimmed();
        }
        return QString::fromUtf8(QJsonDocument(inbound).toJson(QJsonDocument::Compact));
    }

    QJsonObject toTemplate(const QJsonObject &inbound)
    {
        QJsonObject result = inbound;

        QJsonObject settings = result.value(px::settings).toObject();
        const QString flow = templateFlow(inbound);
        QJsonObject client { { px::id, QString::fromLatin1(autoValue) } };
        if (!flow.isEmpty()) {
            client[px::flow] = flow;
        }
        settings[px::clients] = QJsonArray { client };
        result[px::settings] = settings;

        QJsonObject stream = result.value(px::streamSettings).toObject();
        if (stream.contains(px::realitySettings)) {
            QJsonObject reality = stream.value(px::realitySettings).toObject();
            if (reality.contains(px::privateKey)) {
                reality[px::privateKey] = QString::fromLatin1(autoValue);
            }
            if (reality.contains(px::shortIds)) {
                reality[px::shortIds] = QJsonArray { QString::fromLatin1(autoValue) };
            }
            stream[px::realitySettings] = reality;
        }
        if (stream.contains(px::tlsSettings)) {
            QJsonObject tls = stream.value(px::tlsSettings).toObject();
            if (tls.contains(px::certificates)) {
                tls[px::certificates] = certificatesToTemplate(tls.value(px::certificates).toArray());
            }
            stream[px::tlsSettings] = tls;
        }
        result[px::streamSettings] = stream;

        logger.info() << "toTemplate: flow" << flow << "network" << stream.value(px::network).toString() << "security"
                      << stream.value(px::security).toString();
        return result;
    }

    QJsonObject inboundFromServerJson(const QJsonObject &serverJson)
    {
        const QJsonArray inbounds = serverJson.value(px::inbounds).toArray();
        for (const QJsonValue &value : inbounds) {
            const QJsonObject inbound = value.toObject();
            if (inbound.value(px::protocol).toString() == QLatin1String(px::protocolVless)) {
                return inbound;
            }
        }
        logger.warning() << "inboundFromServerJson: no VLESS inbound among" << QString::number(inbounds.size());
        return {};
    }

    QJsonObject toServerJson(const QJsonObject &inboundTemplate, const Secrets &secrets)
    {
        QJsonObject inbound = inboundTemplate;

        QJsonObject settings = inbound.value(px::settings).toObject();
        const QString flow = templateFlow(inboundTemplate);
        settings[px::clients] =
                XrayServerConfig::applyFlowToClients(secrets.clients, flow, XrayClientListFilter::DropWithoutId);
        inbound[px::settings] = settings;

        QJsonObject stream = inbound.value(px::streamSettings).toObject();
        if (stream.contains(px::realitySettings)) {
            QJsonObject reality = stream.value(px::realitySettings).toObject();
            if (isAuto(reality.value(px::privateKey))) {
                reality[px::privateKey] = secrets.realityPrivateKey;
            }
            reality[px::shortIds] = replaceAutoShortIds(reality.value(px::shortIds).toArray(), secrets.realityShortId);
            stream[px::realitySettings] = reality;
        }
        if (stream.contains(px::tlsSettings)) {
            QJsonObject tls = stream.value(px::tlsSettings).toObject();
            tls[px::certificates] = replaceAutoCertificates(tls.value(px::certificates).toArray());
            stream[px::tlsSettings] = tls;
        }
        inbound[px::streamSettings] = stream;

        QJsonObject serverJson;
        serverJson[px::logBlock] = QJsonObject { { px::logLevel, px::logLevelError } };
        serverJson[px::inbounds] = QJsonArray { inbound };
        serverJson[px::outbounds] = QJsonArray { QJsonObject { { px::protocol, px::protocolFreedom } } };

        logger.info() << "toServerJson: clients" << QString::number(settings.value(px::clients).toArray().size())
                      << "flow" << flow << "network" << stream.value(px::network).toString() << "security"
                      << stream.value(px::security).toString() << "port"
                      << QString::number(templatePort(inboundTemplate));
        return serverJson;
    }

    QString templateFlow(const QJsonObject &inboundTemplate)
    {
        const QJsonArray clients = inboundTemplate.value(px::settings).toObject().value(px::clients).toArray();
        return clients.isEmpty() ? QString() : clients.first().toObject().value(px::flow).toString();
    }

    int templatePort(const QJsonObject &inboundTemplate)
    {
        return inboundTemplate.value(px::port).toInt();
    }

    QString templateFromServerConfig(const XrayServerConfig &serverConfig)
    {
        XrayServerConfig srv = serverConfig;
        srv.applyDefaults();

        XrayServerInboundInputs inputs;
        inputs.clients = QJsonArray { XrayServerConfig::makeClientEntry(QString::fromLatin1(autoValue),
                                                                        xrayEffective::serverFlow(srv)) };
        inputs.realityPrivateKey = QString::fromLatin1(autoValue);
        inputs.realityShortId = QString::fromLatin1(autoValue);

        const QJsonObject inbound = inboundFromServerJson(srv.toServerInboundJson(inputs));
        const QString text = format(toTemplate(inbound));
        logger.info() << "templateFromServerConfig: port" << srv.port << "transport" << srv.transport << "security"
                      << srv.security << "flow" << srv.flow << "site" << srv.site;
        return text;
    }

    bool applyTemplate(const QString &templateText, XrayServerConfig &serverConfig)
    {
        const QJsonObject inbound = parse(templateText);
        if (inbound.isEmpty()) {
            logger.warning() << "applyTemplate: template is not a JSON object";
            return false;
        }

        QJsonObject parseable = inbound;
        QJsonObject stream = parseable.value(px::streamSettings).toObject();
        QJsonObject reality = stream.value(px::realitySettings).toObject();
        if (!reality.isEmpty() && !reality.contains(px::dest) && reality.contains(px::target)) {
            reality[px::dest] = reality.value(px::target);
            stream[px::realitySettings] = reality;
            parseable[px::streamSettings] = stream;
        }

        XrayServerConfig derived;
        derived.transportProto = serverConfig.transportProto;
        derived.subnetAddress = serverConfig.subnetAddress;
        derived.settingsVersion = protocols::xray::settingsVersionCurrent;
        derived.fingerprint = serverConfig.fingerprint;
        derived.presetId = serverConfig.presetId;

        const XrayServerJsonStatus status = XrayServerConfig::fromServerInboundJson(
                QJsonObject { { px::inbounds, QJsonArray { parseable } } }, derived);
        if (status != XrayServerJsonStatus::Ok) {
            logger.warning() << "applyTemplate: fromServerInboundJson failed, status"
                             << QString::number(static_cast<int>(status));
            return false;
        }
        if (derived.site.isEmpty()) {
            derived.site = serverConfig.site;
        }
        derived.inboundTemplate = templateText;
        serverConfig = derived;

        logger.info() << "applyTemplate: preset" << serverConfig.presetId << "port" << serverConfig.port << "transport"
                      << serverConfig.transport << "security" << serverConfig.security << "flow" << serverConfig.flow
                      << "site" << serverConfig.site << "sni" << serverConfig.sni;
        return true;
    }

} // namespace amnezia::xrayInbound

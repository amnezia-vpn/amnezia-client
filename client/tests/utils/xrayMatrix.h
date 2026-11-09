#ifndef XRAY_TEST_MATRIX_H
#define XRAY_TEST_MATRIX_H

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

#include "core/models/protocols/xrayProtocolConfig.h"

namespace xraytest
{
    struct MatrixCase
    {
        QString name;
        amnezia::XrayServerConfig srv;
        bool allowedByUi = true;
    };

    enum class RealityHello {
        Connects,
        RealCertificate,
        NoTls13,
        ConfigRejected,
        ClientPanics,
        Unreliable,
    };

    struct RealityFingerprintCase
    {
        QString name;
        RealityHello hello;
    };

    inline QList<RealityFingerprintCase> realityFingerprintCases()
    {
        QList<RealityFingerprintCase> cases;
        auto add = [&cases](RealityHello hello, const QStringList &names) {
            for (const QString &name : names)
                cases.append({ name, hello });
        };
        add(RealityHello::Connects,
            { "chrome", "firefox", "safari", "hellochrome_131", "hellochrome_133", "hellochrome_auto",
              "hellofirefox_148", "hellofirefox_auto", "hellosafari_26_3", "hellosafari_auto" });
        add(RealityHello::RealCertificate,
            { "ios",              "edge",             "qq",               "hellofirefox_120",   "hellochrome_120",
              "helloios_13",      "helloios_14",      "helloedge_106",    "hello360_11_0",      "helloqq_11_1",
              "hellofirefox_63",  "hellofirefox_65",  "hellofirefox_99",  "hellofirefox_102",   "hellofirefox_105",
              "hellochrome_70",   "hellochrome_72",   "hellochrome_83",   "hellochrome_87",     "hellochrome_96",
              "hellochrome_100",  "hellochrome_102",  "hellochrome_106_shuffle", "helloios_auto", "helloedge_85",
              "helloedge_auto",   "hellosafari_16_0", "helloqq_auto",     "hellochrome_115_pq", "hellochrome_120_pq" });
        add(RealityHello::NoTls13,
            { "android", "360", "hellofirefox_55", "hellofirefox_56", "hellochrome_58", "hellochrome_62",
              "helloios_11_1", "helloios_12_1", "helloandroid_11_okhttp", "hello360_auto", "hello360_7_5" });
        add(RealityHello::ConfigRejected, { "unsafe", "hellogolang" });
        add(RealityHello::ClientPanics,
            { "hellochrome_100_psk", "hellochrome_112_psk_shuf", "hellochrome_114_padding_psk_shuf",
              "hellochrome_115_pq_psk" });
        add(RealityHello::Unreliable,
            { "random", "randomized", "randomizednoalpn", "hellorandomized", "hellorandomizedalpn",
              "hellorandomizednoalpn" });
        return cases;
    }

    inline QStringList legacyRealityFingerprints()
    {
        return { "hellochrome_120", "ios",    "android", "edge",    "360",     "qq",
                 "random",          "",       " Chrome ", "hellogolang", "unsafe", "garbage",
                 "hellochrome_100_psk" };
    }

    inline void editStream(QJsonObject &config, const char *bounds, const std::function<void(QJsonObject &)> &edit)
    {
        QJsonArray list = config.value(QString::fromLatin1(bounds)).toArray();
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject bound = list.at(i).toObject();
            if (bound.value("protocol").toString() != QLatin1String("vless"))
                continue;
            QJsonObject stream = bound.value("streamSettings").toObject();
            edit(stream);
            bound["streamSettings"] = stream;
            list[i] = bound;
        }
        config[QString::fromLatin1(bounds)] = list;
    }

    inline void editSection(QJsonObject &config, const char *bounds, const char *section,
                            const std::function<void(QJsonObject &)> &edit)
    {
        editStream(config, bounds, [&](QJsonObject &stream) {
            QJsonObject settings = stream.value(QString::fromLatin1(section)).toObject();
            edit(settings);
            stream[QString::fromLatin1(section)] = settings;
        });
    }

    inline void setClientRealityFingerprint(QJsonObject &client, const QString &fingerprint)
    {
        editSection(client, "outbounds", "realitySettings",
                    [&fingerprint](QJsonObject &reality) { reality["fingerprint"] = fingerprint; });
    }

    inline QString clientId()
    {
        return QStringLiteral("11111111-2222-4333-8444-555555555555");
    }

    inline QString key32(char fill)
    {
        return QString::fromLatin1(QByteArray(32, fill).toBase64(QByteArray::Base64UrlEncoding
                                                                 | QByteArray::OmitTrailingEquals));
    }

    inline QString shortId()
    {
        return QStringLiteral("0123456789abcdef");
    }

    inline QString tlsPin()
    {
        return QString(64, QLatin1Char('a'));
    }

    inline amnezia::XrayServerConfig baseConfig(const QString &transport, const QString &security, const QString &flow)
    {
        amnezia::XrayServerConfig srv;
        srv.port = QStringLiteral("443");
        srv.transportProto = QStringLiteral("tcp");
        srv.transport = transport;
        srv.security = security;
        srv.flow = flow;
        srv.sni = QStringLiteral("www.example.com");
        srv.site = QStringLiteral("www.example.com");
        srv.applyDefaults(false);
        srv.settingsVersion = amnezia::protocols::xray::settingsVersionCurrent;
        return srv;
    }

    inline QList<MatrixCase> matrix()
    {
        const QStringList transports = { "raw", "xhttp", "mkcp" };
        const QStringList securities = { "tls", "reality" };
        const QStringList flows = { "", "xtls-rprx-vision", "xtls-rprx-vision-udp443" };
        const QStringList modes = { "Auto", "Packet-up", "Stream-up", "Stream-one" };
        const QStringList placements = { "Body", "Auto", "Header", "Cookie" };

        QList<MatrixCase> cases;
        for (const QString &transport : transports) {
            for (const QString &security : securities) {
                for (const QString &flow : flows) {
                    const QString flowName = flow.isEmpty() ? QStringLiteral("noflow") : flow;
                    if (transport != QLatin1String("xhttp")) {
                        cases.append({ QStringLiteral("%1/%2/%3").arg(transport, security, flowName),
                                       baseConfig(transport, security, flow),
                                       transport != QLatin1String("mkcp") || security != QLatin1String("reality") });
                        continue;
                    }
                    for (const QString &mode : modes) {
                        for (const QString &placement : placements) {
                            for (const bool obfs : { true, false }) {
                                MatrixCase c;
                                c.srv = baseConfig(transport, security, flow);
                                c.srv.xhttp.mode = mode;
                                c.srv.xhttp.uplinkDataPlacement = placement;
                                c.srv.xhttp.xPadding.obfsMode = obfs;
                                c.srv.xhttp.uplinkMethod = placement == QLatin1String("Header") ? QStringLiteral("GET")
                                                                                                 : QStringLiteral("PUT");
                                c.srv.xhttp.path = placement == QLatin1String("Cookie") ? QStringLiteral("text")
                                                                                         : QStringLiteral("/p");
                                auto &xmux = c.srv.xhttp.xmux;
                                xmux.enabled = placement != QLatin1String("Auto");
                                if (placement == QLatin1String("Body")) {
                                    xmux.maxConcurrencyMin = QStringLiteral("16");
                                    xmux.maxConcurrencyMax = QStringLiteral("32");
                                } else {
                                    xmux.maxConnectionsMin = QStringLiteral("4");
                                    xmux.maxConnectionsMax = QStringLiteral("4");
                                }
                                if (!obfs) {
                                    c.srv.xhttp.xPadding.bytesMin = QStringLiteral("200");
                                    c.srv.xhttp.xPadding.bytesMax = QStringLiteral("400");
                                }
                                c.name = QStringLiteral("xhttp/%1/%2/%3/%4/obfs-%5")
                                                 .arg(security, flowName, mode, placement,
                                                      obfs ? QStringLiteral("on") : QStringLiteral("off"));
                                cases.append(c);
                            }
                        }
                    }
                }
            }
        }

        MatrixCase bothXmux;
        bothXmux.srv = baseConfig("xhttp", "tls", "");
        bothXmux.srv.xhttp.xmux.maxConcurrencyMin = QStringLiteral("1");
        bothXmux.srv.xhttp.xmux.maxConcurrencyMax = QStringLiteral("1");
        bothXmux.srv.xhttp.xmux.maxConnectionsMin = QStringLiteral("1");
        bothXmux.srv.xhttp.xmux.maxConnectionsMax = QStringLiteral("1");
        bothXmux.allowedByUi = false;
        bothXmux.name = QStringLiteral("xhttp/tls/xmux-concurrency-and-connections");
        cases.append(bothXmux);

        return cases;
    }

    inline amnezia::XrayServerInboundInputs inboundInputs(const amnezia::XrayServerConfig &srv)
    {
        amnezia::XrayServerInboundInputs in;
        in.clients = QJsonArray { amnezia::XrayServerConfig::makeClientEntry(clientId(),
                                                                             amnezia::xrayEffective::serverFlow(srv)) };
        in.realityPrivateKey = key32('\x01');
        in.realityShortId = shortId();
        return in;
    }

    inline amnezia::XrayClientOutboundInputs outboundInputs()
    {
        amnezia::XrayClientOutboundInputs in;
        in.serverAddress = QStringLiteral("203.0.113.10");
        in.clientId = clientId();
        in.realityPublicKey = key32('\x02');
        in.realityShortId = shortId();
        in.tlsPinnedPeerCertSha256 = tlsPin();
        return in;
    }

    inline QJsonObject serverJson(const amnezia::XrayServerConfig &srv)
    {
        return srv.toServerInboundJson(inboundInputs(srv));
    }

    inline QJsonObject clientJson(const amnezia::XrayServerConfig &srv)
    {
        amnezia::XrayProtocolConfig cfg;
        cfg.serverConfig = srv;
        return cfg.toClientOutboundJson(outboundInputs());
    }

    inline QJsonObject firstInbound(const QJsonObject &config)
    {
        return config.value(QStringLiteral("inbounds")).toArray().first().toObject();
    }

    inline QJsonObject vlessEntry(const QJsonObject &config, const QString &list)
    {
        for (const QJsonValue &v : config.value(list).toArray()) {
            if (v.toObject().value(QStringLiteral("protocol")).toString() == QLatin1String("vless"))
                return v.toObject();
        }
        return {};
    }

    inline QJsonObject vlessOutbound(const QJsonObject &config)
    {
        return vlessEntry(config, QStringLiteral("outbounds"));
    }

    inline QJsonObject streamOf(const QJsonObject &bound)
    {
        return bound.value(QStringLiteral("streamSettings")).toObject();
    }

    inline bool containsKeyDeep(const QJsonValue &value, const QString &key)
    {
        if (value.isObject()) {
            const QJsonObject obj = value.toObject();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.key() == key || containsKeyDeep(it.value(), key))
                    return true;
            }
        } else if (value.isArray()) {
            for (const QJsonValue &v : value.toArray()) {
                if (containsKeyDeep(v, key))
                    return true;
            }
        }
        return false;
    }

    inline QString compact(const QJsonObject &obj)
    {
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

#endif

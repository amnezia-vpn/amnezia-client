#ifndef XRAY_EXPLORE_CASES_H
#define XRAY_EXPLORE_CASES_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <functional>

#include "core/models/protocols/xrayInbound.h"
#include "core/utils/constants/protocolConstants.h"
#include "utils/xrayContractCases.h"
#include "utils/xrayMatrix.h"

namespace xrayexplore
{
    using xraytest::InboundEdit;
    using ClientEdit = std::function<void(QJsonObject &)>;
    using Range = QPair<QString, QString>;

    struct Carrier
    {
        QString name;
        QString transport;
        QString security;
        QString flow;
        QString mode;
    };

    struct ExploreCase
    {
        QString group;
        QString name;
        Carrier carrier;
        InboundEdit edit;
        QString fingerprint;
        ClientEdit clientEdit;
    };

    inline Carrier raw(const QString &security, const QString &flow)
    {
        const QString flowName = flow.isEmpty() ? QStringLiteral("noflow") : flow;
        return { QStringLiteral("raw/%1/%2").arg(security, flowName), QStringLiteral("raw"), security, flow, {} };
    }

    inline Carrier xhttp(const QString &security, const QString &mode)
    {
        return { QStringLiteral("xhttp/%1/%2").arg(security, mode), QStringLiteral("xhttp"), security, {}, mode };
    }

    inline Carrier mkcp()
    {
        return { QStringLiteral("mkcp/tls"), QStringLiteral("mkcp"), QStringLiteral("tls"), {}, {} };
    }

    inline QList<Carrier> carriers(const QString &security)
    {
        QList<Carrier> out;
        for (const QString &flow : { QString(), QStringLiteral("xtls-rprx-vision") })
            out << raw(security, flow);
        for (const QString &mode : xraytest::xhttpModeOptions())
            out << xhttp(security, mode);
        return out;
    }

    inline QJsonObject carrierInbound(const Carrier &carrier, const QString &port, const QString &site)
    {
        amnezia::XrayServerConfig srv = xraytest::baseConfig(carrier.transport, carrier.security, carrier.flow);
        srv.port = port;
        srv.sni = site;
        srv.site = site;
        if (!carrier.mode.isEmpty())
            srv.xhttp.mode = carrier.mode;
        return amnezia::xrayInbound::parse(amnezia::xrayInbound::templateFromServerConfig(srv));
    }

    inline QJsonValue typed(const QString &value)
    {
        if (value.isEmpty())
            return QJsonValue(QJsonValue::Undefined);
        bool isNumber = false;
        const qlonglong number = value.toLongLong(&isNumber);
        if (isNumber && QString::number(number) == value)
            return QJsonValue(number);
        return value;
    }

    inline QJsonValue alpnValue(const QString &value)
    {
        QJsonArray entries;
        for (const QString &entry : value.split(QLatin1Char(','), Qt::SkipEmptyParts))
            entries.append(entry);
        return entries;
    }

    inline QString jsonPlacement(const QString &uiName)
    {
        QString lower = uiName.toLower();
        return lower == QLatin1String("query in header") ? QStringLiteral("queryInHeader") : lower;
    }

    inline InboundEdit realityTarget(const QString &host)
    {
        return [host](QJsonObject &inbound) {
            QJsonObject stream = inbound.value(QStringLiteral("streamSettings")).toObject();
            QJsonObject reality = stream.value(QStringLiteral("realitySettings")).toObject();
            const QString key =
                    reality.contains(QStringLiteral("target")) ? QStringLiteral("target") : QStringLiteral("dest");
            reality[key] = host + QStringLiteral(":443");
            stream[QStringLiteral("realitySettings")] = reality;
            inbound[QStringLiteral("streamSettings")] = stream;
        };
    }

    inline InboundEdit serverName(const Carrier &carrier, const QString &name)
    {
        if (carrier.security == QLatin1String("reality"))
            return xraytest::realityServerName(name);
        return xraytest::tlsValue(QStringLiteral("serverName"), name);
    }

    inline QString shown(const QString &value)
    {
        return QStringLiteral("'%1'").arg(value);
    }

    inline QString shown(const Range &range)
    {
        return QStringLiteral("'%1'..'%2'").arg(range.first, range.second);
    }

    class Builder
    {
    public:
        QList<ExploreCase> cases;

        void add(const QString &group, const Carrier &carrier, const QString &what, const InboundEdit &edit,
                 const QString &fingerprint = QString(), const ClientEdit &clientEdit = {})
        {
            const QString name = what.isEmpty() ? QStringLiteral("%1/%2").arg(group, carrier.name)
                                                : QStringLiteral("%1/%2 %3").arg(group, carrier.name, what);
            cases.append({ group, name, carrier, edit, fingerprint, clientEdit });
        }

        void values(const QString &group, const QList<Carrier> &on, const QString &field,
                    const std::function<InboundEdit(const QString &)> &setter, const QStringList &list,
                    const InboundEdit &prepare = {})
        {
            for (const Carrier &carrier : on) {
                for (const QString &value : list)
                    add(group, carrier, field + QLatin1Char('=') + shown(value), withPrepare(prepare, setter(value)));
            }
        }

        void ranges(const QString &group, const QList<Carrier> &on, const QString &field,
                    const std::function<InboundEdit(const QJsonValue &)> &setter, const QList<Range> &list,
                    const InboundEdit &prepare = {})
        {
            for (const Carrier &carrier : on) {
                for (const Range &range : list) {
                    add(group, carrier, field + QLatin1Char('=') + shown(range),
                        withPrepare(prepare, setter(xraytest::rangeValue(range.first, range.second))));
                }
            }
        }

        void fingerprints(const QString &group, const QList<Carrier> &on, const QStringList &list)
        {
            for (const Carrier &carrier : on) {
                for (const QString &fingerprint : list)
                    add(group, carrier, QStringLiteral("stored fingerprint=") + shown(fingerprint), {}, fingerprint);
            }
        }

    private:
        static InboundEdit withPrepare(const InboundEdit &prepare, const InboundEdit &edit)
        {
            return prepare ? xraytest::both(prepare, edit) : edit;
        }
    };

    inline std::function<InboundEdit(const QString &)> xhttpText(const QString &key)
    {
        return [key](const QString &value) { return xraytest::xhttpValue(key, typed(value)); };
    }

    inline std::function<InboundEdit(const QString &)> kcpText(const QString &key)
    {
        return [key](const QString &value) { return xraytest::kcpValue(key, typed(value)); };
    }

    inline std::function<InboundEdit(const QJsonValue &)> xhttpRange(const QString &key)
    {
        return [key](const QJsonValue &value) { return xraytest::xhttpValue(key, value); };
    }

    inline std::function<InboundEdit(const QJsonValue &)> xmuxRange(const QString &key)
    {
        return [key](const QJsonValue &value) { return xraytest::xmuxValue(key, value); };
    }

    inline QString longDomain(int length)
    {
        QStringList labels;
        int left = length - 4;
        char letter = 'a';
        while (left > 0) {
            const int size = qMin(63, left);
            labels << QString(size, QLatin1Char(letter++));
            left -= size + 1;
        }
        return labels.join(QLatin1Char('.')) + QStringLiteral(".com");
    }

    inline void baseGroup(Builder &b)
    {
        const QString g = QStringLiteral("base");
        for (const QString &security : { QStringLiteral("tls"), QStringLiteral("reality") }) {
            for (const Carrier &carrier : carriers(security))
                b.add(g, carrier, {}, {});
        }
        b.add(g, mkcp(), {}, {});
        b.add(g, raw(QStringLiteral("reality"), {}), QStringLiteral("network=kcp"),
              xraytest::streamValue(QStringLiteral("network"), QStringLiteral("kcp")));
        for (const Carrier &carrier : { raw(QStringLiteral("tls"), QStringLiteral("xtls-rprx-vision")),
                                        raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")) }) {
            b.add(g, carrier, QStringLiteral("client flow=xtls-rprx-vision-udp443"),
                  xraytest::clientFlow(QStringLiteral("xtls-rprx-vision-udp443")));
        }
        b.values(g, { raw(QStringLiteral("tls"), {}) }, QStringLiteral("security"),
                 [](const QString &value) { return xraytest::streamValue(QStringLiteral("security"), value); },
                 { "none", "", "xtls", "TLS", "garbage" });
    }

    inline QStringList tlsFingerprints()
    {
        QStringList fingerprints = xraytest::fingerprintOptions();
        fingerprints << QString::fromLatin1(amnezia::protocols::xray::defaultFingerprint);
        fingerprints.removeDuplicates();
        return fingerprints;
    }

    inline void tlsFingerprintGrid(Builder &b, const QString &g, const QList<Carrier> &bases)
    {
        for (const Carrier &carrier : bases) {
            for (const QString &fingerprint : tlsFingerprints()) {
                for (const QString &alpn : xraytest::alpnOptions()) {
                    b.add(g, carrier, QStringLiteral("stored fingerprint=%1 alpn=%2").arg(fingerprint, alpn),
                          xraytest::tlsValue(QStringLiteral("alpn"), alpnValue(alpn)), fingerprint);
                }
            }
        }
    }

    inline void tlsGroup(Builder &b)
    {
        const QString g = QStringLiteral("tls");
        tlsFingerprintGrid(b, g,
                           { raw(QStringLiteral("tls"), {}),
                             raw(QStringLiteral("tls"), QStringLiteral("xtls-rprx-vision")),
                             xhttp(QStringLiteral("tls"), QStringLiteral("Stream-one")) });
        const auto alpn = [](const QString &value) {
            return xraytest::tlsValue(QStringLiteral("alpn"), alpnValue(value));
        };
        b.values(g, { mkcp() }, QStringLiteral("alpn"), alpn, xraytest::alpnOptions());
        b.values(g, { raw(QStringLiteral("tls"), {}) }, QStringLiteral("alpn"), alpn,
                 { "HTTP/2", "h2, http/1.1", "http/1.1,h2", "h3", "foo", "" });
        b.fingerprints(g, { raw(QStringLiteral("tls"), {}) }, { "", "foo", " Chrome " });
    }

    inline void tlsXhttpGroup(Builder &b)
    {
        tlsFingerprintGrid(b, QStringLiteral("tlsxhttp"),
                           { xhttp(QStringLiteral("tls"), QStringLiteral("Auto")),
                             xhttp(QStringLiteral("tls"), QStringLiteral("Packet-up")),
                             xhttp(QStringLiteral("tls"), QStringLiteral("Stream-up")) });
    }

    inline void realityGroup(Builder &b)
    {
        const QString g = QStringLiteral("reality");
        QStringList fingerprints = xraytest::realityFingerprintOptions();
        for (const xraytest::RealityFingerprintCase &fp : xraytest::realityFingerprintCases()) {
            if (fp.hello == xraytest::RealityHello::Connects)
                fingerprints << fp.name;
        }
        fingerprints.removeDuplicates();
        b.fingerprints(g, carriers(QStringLiteral("reality")), fingerprints);
        b.fingerprints(g, { raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")) },
                       xraytest::legacyRealityFingerprints());
    }

    inline void oldClientGroup(Builder &b)
    {
        const QString g = QStringLiteral("oldclient");
        for (const Carrier &carrier : { raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")),
                                        xhttp(QStringLiteral("reality"), QStringLiteral("Auto")) }) {
            for (const xraytest::RealityFingerprintCase &fp : xraytest::realityFingerprintCases()) {
                const QString name = fp.name;
                b.add(g, carrier, QStringLiteral("client fingerprint=%1").arg(name), {}, QString(),
                      [name](QJsonObject &client) { xraytest::setClientRealityFingerprint(client, name); });
            }
        }
    }

    inline void sniGroup(Builder &b)
    {
        const QString g = QStringLiteral("sni");
        const QList<Carrier> bases = { raw(QStringLiteral("tls"), {}),
                                       xhttp(QStringLiteral("tls"), QStringLiteral("Stream-one")), mkcp(),
                                       raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")),
                                       xhttp(QStringLiteral("reality"), QStringLiteral("Auto")) };
        const QStringList names = { "www.googletagmanager.com",
                                    "www.microsoft.com",
                                    "www.apple.com",
                                    "dl.google.com",
                                    "www.cloudflare.com",
                                    "github.com",
                                    "www.bing.com",
                                    "yandex.ru",
                                    "",
                                    "1.1.1.1",
                                    "a_b.example.com",
                                    longDomain(253),
                                    "amnezia-explore-nonexistent.com",
                                    "neverssl.com",
                                    "*.example.com",
                                    "localhost",
                                    "1234",
                                    longDomain(254),
                                    "xn--e1afmkfd.xn--p1ai",
                                    "example.com.",
                                    QString(63, QLatin1Char('a')) + QStringLiteral(".example.com"),
                                    "2606:4700::1111",
                                    " example.com " };
        for (const Carrier &carrier : bases) {
            for (const QString &name : names)
                b.add(g, carrier, QStringLiteral("serverName=") + shown(name), serverName(carrier, name));
        }
        const Carrier reality = raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
        b.add(g, reality, QStringLiteral("serverNames='www.microsoft.com' target='www.googletagmanager.com'"),
              xraytest::both(xraytest::realityServerName(QStringLiteral("www.microsoft.com")),
                             realityTarget(QStringLiteral("www.googletagmanager.com"))));
        b.add(g, reality, QStringLiteral("serverNames='' target='www.microsoft.com'"),
              xraytest::both(xraytest::realityServerName(QString()), realityTarget(QStringLiteral("www.microsoft.com"))));
    }

    inline void portGroup(Builder &b)
    {
        const QString g = QStringLiteral("port");
        const auto port = [](const QString &value) { return xraytest::put({ QStringLiteral("port") }, typed(value)); };
        b.values(g, { raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")), mkcp() },
                 QStringLiteral("port"), port, { "2053", "65535", "1" });
        b.values(g, { raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")) }, QStringLiteral("port"),
                 port, { "0", "65536", "-1", "", "abc" });
    }

    inline void xhttpGroup(Builder &b)
    {
        const QString g = QStringLiteral("xhttp");
        auto inMode = [](const QString &mode) {
            return QList<Carrier> { xhttp(QStringLiteral("tls"), mode), xhttp(QStringLiteral("reality"), mode) };
        };
        QList<Carrier> everyMode;
        for (const QString &mode : xraytest::xhttpModeOptions())
            everyMode << inMode(mode);
        const QList<Carrier> streamOne = inMode(QStringLiteral("Stream-one"));
        const QList<Carrier> packetUp = inMode(QStringLiteral("Packet-up"));
        const QList<Carrier> streamUp = inMode(QStringLiteral("Stream-up"));
        const QList<Range> xmuxRanges = { { "0", "0" }, { "1", "1" },   { "16", "32" }, { "0", "4" },
                                          { "4", "0" }, { "32", "16" }, { "abc", "5" } };
        auto placement = [](const QString &key, const QString &value) { return xraytest::xhttpValue(key, value); };
        auto lowered = [](QStringList list) {
            for (QString &item : list)
                item = jsonPlacement(item);
            return list;
        };

        b.values(g, streamOne, QStringLiteral("mode"), xhttpText(QStringLiteral("mode")),
                 { "", "packet-up", "Packet-up", "foo" });
        b.values(g, streamOne, QStringLiteral("host"), xhttpText(QStringLiteral("host")),
                 { "", "www.googletagmanager.com", "example.com", "1.2.3.4", "::1", "a,b", "123", "bad host" });
        b.values(g, streamOne, QStringLiteral("path"), xhttpText(QStringLiteral("path")),
                 { "", "/", "/abc", "abc", " /abc ", "/a/b", "/x?y=1", "/%41", "/a b" });

        QStringList methods = xraytest::xhttpUplinkMethodOptions();
        methods << "post" << "GET" << "OPTIONS" << "FOO";
        b.values(g, everyMode, QStringLiteral("uplinkHTTPMethod"), xhttpText(QStringLiteral("uplinkHTTPMethod")),
                 methods);

        for (const Carrier &carrier : everyMode) {
            for (const bool grpc : { true, false }) {
                for (const bool sse : { true, false }) {
                    b.add(g, carrier,
                          QStringLiteral("noGRPCHeader=%1 noSSEHeader=%2")
                                  .arg(grpc ? QStringLiteral("true") : QStringLiteral("false"),
                                       sse ? QStringLiteral("true") : QStringLiteral("false")),
                          xraytest::both(xraytest::xhttpValue(QStringLiteral("noGRPCHeader"), grpc),
                                         xraytest::xhttpValue(QStringLiteral("noSSEHeader"), sse)));
                }
            }
        }

        QStringList idPlacements = lowered(xraytest::xhttpSessionPlacementOptions());
        idPlacements << "" << "body" << "Header" << "foo";
        b.values(g, everyMode, QStringLiteral("sessionIDPlacement"), xhttpText(QStringLiteral("sessionIDPlacement")),
                 idPlacements);
        b.values(g, everyMode, QStringLiteral("seqPlacement"), xhttpText(QStringLiteral("seqPlacement")), idPlacements);
        const QStringList keys = { "", "sid", "X-Sid", "a-b_1", "a b", "a:b" };
        b.values(g, packetUp, QStringLiteral("sessionIDPlacement=header sessionIDKey"),
                 xhttpText(QStringLiteral("sessionIDKey")), keys,
                 placement(QStringLiteral("sessionIDPlacement"), QStringLiteral("header")));
        b.values(g, packetUp, QStringLiteral("seqPlacement=header seqKey"), xhttpText(QStringLiteral("seqKey")), keys,
                 placement(QStringLiteral("seqPlacement"), QStringLiteral("header")));

        QStringList dataPlacements = lowered(xraytest::xhttpUplinkDataPlacementOptions());
        dataPlacements << "" << "Body" << "foo";
        b.values(g, everyMode, QStringLiteral("uplinkDataPlacement"), xhttpText(QStringLiteral("uplinkDataPlacement")),
                 dataPlacements);
        const InboundEdit headerData = placement(QStringLiteral("uplinkDataPlacement"), QStringLiteral("header"));
        b.values(g, packetUp, QStringLiteral("uplinkDataPlacement=header uplinkDataKey"),
                 xhttpText(QStringLiteral("uplinkDataKey")), { "", "xd", "X-Data", "a:b" }, headerData);
        b.values(g, packetUp, QStringLiteral("uplinkDataPlacement=header uplinkChunkSize"),
                 xhttpText(QStringLiteral("uplinkChunkSize")), { "", "0", "1", "63", "64", "4096", "65536", "abc" },
                 headerData);

        b.values(g, packetUp, QStringLiteral("scMaxBufferedPosts"), xhttpText(QStringLiteral("scMaxBufferedPosts")),
                 { "", "0", "1", "30", "1000", "-1", "abc" });
        b.ranges(g, packetUp, QStringLiteral("scMaxEachPostBytes"), xhttpRange(QStringLiteral("scMaxEachPostBytes")),
                 { { "1000000", "1000000" },
                   { "100", "100" },
                   { "1", "1" },
                   { "0", "0" },
                   { "0", "1000" },
                   { "1000000", "0" },
                   { "2000", "1000" },
                   { "", "5000" },
                   { "abc", "5" } });
        b.ranges(g, packetUp, QStringLiteral("scMinPostsIntervalMs"), xhttpRange(QStringLiteral("scMinPostsIntervalMs")),
                 { { "100", "800" }, { "0", "0" }, { "0", "500" }, { "10", "10" }, { "800", "100" }, { "abc", "5" } });
        b.ranges(g, streamUp, QStringLiteral("scStreamUpServerSecs"), xhttpRange(QStringLiteral("scStreamUpServerSecs")),
                 { { "1", "100" }, { "0", "0" }, { "0", "5" }, { "1", "0" }, { "5", "5" }, { "100", "1" } });

        for (const Carrier &carrier : streamOne) {
            b.add(g, carrier, QStringLiteral("xPaddingObfsMode=false"),
                  xraytest::xhttpValue(QStringLiteral("xPaddingObfsMode"), false));
        }
        b.ranges(g, streamOne, QStringLiteral("xPaddingBytes"), xhttpRange(QStringLiteral("xPaddingBytes")),
                 { { "", "" },
                   { "100", "1000" },
                   { "0", "0" },
                   { "0", "1" },
                   { "0", "1000" },
                   { "1", "1" },
                   { "500", "100" },
                   { "100", "" },
                   { "-1", "5" } });
        b.values(g, streamOne, QStringLiteral("xPaddingKey"), xhttpText(QStringLiteral("xPaddingKey")),
                 { "", "pad", "X-Pad", "a b" });
        b.values(g, streamOne, QStringLiteral("xPaddingPlacement=header xPaddingHeader"),
                 xhttpText(QStringLiteral("xPaddingHeader")), { "", "pad", "X-Pad", "a b" },
                 placement(QStringLiteral("xPaddingPlacement"), QStringLiteral("header")));
        for (const QString &paddingPlacement : lowered(xraytest::xPaddingPlacementOptions())) {
            b.values(g, streamOne, QStringLiteral("xPaddingPlacement=%1 xPaddingMethod").arg(paddingPlacement),
                     xhttpText(QStringLiteral("xPaddingMethod")), lowered(xraytest::xPaddingMethodOptions()),
                     placement(QStringLiteral("xPaddingPlacement"), paddingPlacement));
        }
        b.values(g, streamOne, QStringLiteral("xPaddingPlacement"), xhttpText(QStringLiteral("xPaddingPlacement")),
                 { "Header", "body", "foo" });
        b.values(g, streamOne, QStringLiteral("xPaddingMethod"), xhttpText(QStringLiteral("xPaddingMethod")),
                 { "Repeat-x", "random", "foo" });

        for (const Carrier &carrier : streamOne) {
            b.add(g, carrier, QStringLiteral("xmux removed"),
                  xraytest::xhttpValue(QStringLiteral("xmux"), QJsonValue(QJsonValue::Undefined)));
            b.add(g, carrier, QStringLiteral("xmux maxConcurrency=1 maxConnections=1"),
                  xraytest::both(xraytest::xmuxValue(QStringLiteral("maxConcurrency"), 1),
                                 xraytest::xmuxValue(QStringLiteral("maxConnections"), 1)));
        }
        for (const QString &key :
             { QStringLiteral("maxConcurrency"), QStringLiteral("maxConnections"), QStringLiteral("cMaxReuseTimes"),
               QStringLiteral("hMaxRequestTimes"), QStringLiteral("hMaxReusableSecs") })
            b.ranges(g, streamOne, QStringLiteral("xmux ") + key, xmuxRange(key), xmuxRanges);
        b.values(g, streamOne, QStringLiteral("xmux hKeepAlivePeriod"),
                 [](const QString &value) {
                     return xraytest::xmuxValue(QStringLiteral("hKeepAlivePeriod"), typed(value));
                 },
                 { "", "0", "30", "-1", "abc" });
    }

    inline void mkcpGroup(Builder &b)
    {
        const QString g = QStringLiteral("mkcp");
        const QList<Carrier> on = { mkcp() };
        b.values(g, on, QStringLiteral("tti"), kcpText(QStringLiteral("tti")),
                 { "", "50", "20", "10", "100", "9", "101", "0", "1000", "abc", "-1" });
        b.values(g, on, QStringLiteral("mtu"), kcpText(QStringLiteral("mtu")),
                 { "", "1350", "576", "1460", "575", "1461", "0", "21", "abc" });
        const QStringList capacities = { "", "5", "0", "1", "4095", "4096", "5000", "abc", "9999999999", "-1" };
        b.values(g, on, QStringLiteral("uplinkCapacity"), kcpText(QStringLiteral("uplinkCapacity")), capacities);
        b.values(g, on, QStringLiteral("downlinkCapacity"), kcpText(QStringLiteral("downlinkCapacity")), capacities);
        b.values(g, on, QStringLiteral("cwndMultiplier"), kcpText(QStringLiteral("cwndMultiplier")),
                 { "", "1", "2", "5000", "5001", "6000", "0", "-5", "abc" });
        b.values(g, on, QStringLiteral("maxSendingWindow"), kcpText(QStringLiteral("maxSendingWindow")),
                 { "", "1350", "1349", "0", "2097152", "abc", "-1" });
        b.add(g, mkcp(), QStringLiteral("mtu=1460 maxSendingWindow=1400"),
              xraytest::both(xraytest::kcpValue(QStringLiteral("mtu"), 1460),
                             xraytest::kcpValue(QStringLiteral("maxSendingWindow"), 1400)));
    }

    inline QStringList groupNames()
    {
        return { "base", "tls", "tlsxhttp", "reality", "oldclient", "sni", "xhttp", "mkcp", "port" };
    }

    inline QList<ExploreCase> cases(const QStringList &groups)
    {
        Builder b;
        const QList<QPair<QString, std::function<void(Builder &)>>> all = {
            { "base", baseGroup },           { "tls", tlsGroup },
            { "tlsxhttp", tlsXhttpGroup },   { "reality", realityGroup },
            { "oldclient", oldClientGroup }, { "sni", sniGroup },
            { "xhttp", xhttpGroup },         { "mkcp", mkcpGroup },
            { "port", portGroup },
        };
        for (const auto &group : all) {
            if (groups.contains(group.first))
                group.second(b);
        }
        return b.cases;
    }
}

#endif

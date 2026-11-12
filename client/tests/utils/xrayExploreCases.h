#ifndef XRAY_EXPLORE_CASES_H
#define XRAY_EXPLORE_CASES_H

#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <functional>

#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayMatrix.h"

namespace xrayexplore
{
    using Edit = std::function<void(XrayConfigModel &)>;
    using ClientEdit = std::function<void(QJsonObject &)>;
    using TextSetter = void (XrayConfigModel::*)(const QString &);
    using Range = QPair<QString, QString>;

    struct ExploreCase
    {
        QString group;
        QString name;
        Edit edit;
        ClientEdit clientEdit;
    };

    struct Carrier
    {
        QString name;
        QString transport;
        QString security;
        QString flow;
        QString mode;
    };

    inline void apply(XrayConfigModel &m, const Carrier &c)
    {
        m.setTransport(c.transport);
        m.setSecurity(c.security);
        m.setFlow(c.flow);
        if (!c.mode.isEmpty())
            m.setXhttpMode(c.mode);
    }

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
        for (const QString &flow : XrayConfigModel::flowOptions())
            out << raw(security, flow);
        for (const QString &mode : XrayConfigModel::xhttpModeOptions())
            out << xhttp(security, mode);
        return out;
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

        void add(const QString &group, const Carrier &carrier, const QString &what, const Edit &edit,
                 const ClientEdit &clientEdit = {})
        {
            const QString name = what.isEmpty() ? QStringLiteral("%1/%2").arg(group, carrier.name)
                                                : QStringLiteral("%1/%2 %3").arg(group, carrier.name, what);
            cases.append({ group, name,
                           [carrier, edit](XrayConfigModel &m) {
                               apply(m, carrier);
                               if (edit)
                                   edit(m);
                           },
                           clientEdit });
        }

        void text(const QString &group, const QList<Carrier> &on, const QString &field, TextSetter setter,
                  const QStringList &values, const Edit &prepare = {})
        {
            for (const Carrier &carrier : on) {
                for (const QString &value : values) {
                    add(group, carrier, field + QLatin1Char('=') + shown(value), [prepare, setter, value](XrayConfigModel &m) {
                        if (prepare)
                            prepare(m);
                        (m.*setter)(value);
                    });
                }
            }
        }

        void range(const QString &group, const QList<Carrier> &on, const QString &field, TextSetter setMin,
                   TextSetter setMax, const QList<Range> &values, const Edit &prepare = {})
        {
            for (const Carrier &carrier : on) {
                for (const Range &value : values) {
                    add(group, carrier, field + QLatin1Char('=') + shown(value),
                        [prepare, setMin, setMax, value](XrayConfigModel &m) {
                            if (prepare)
                                prepare(m);
                            (m.*setMin)(value.first);
                            (m.*setMax)(value.second);
                        });
                }
            }
        }
    };

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
        for (const QString &security : XrayConfigModel::securityOptions()) {
            for (const Carrier &carrier : carriers(security))
                b.add(g, carrier, {}, {});
        }
        b.add(g, mkcp(), {}, {});
        b.add(g, mkcp(), QStringLiteral("then security=reality"),
              [](XrayConfigModel &m) { m.setSecurity(QStringLiteral("reality")); });
        b.add(g, raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")),
              QStringLiteral("then transport=mkcp"), [](XrayConfigModel &m) { m.setTransport(QStringLiteral("mkcp")); });
        b.text(g, { raw(QStringLiteral("tls"), {}) }, QStringLiteral("security"), &XrayConfigModel::setSecurity,
               { "none", "", "xtls", "TLS", "garbage" });
    }

    inline void tlsFingerprintGrid(Builder &b, const QString &g, const QList<Carrier> &bases)
    {
        QStringList fingerprints = XrayConfigModel::fingerprintOptions();
        fingerprints << QString::fromLatin1(amnezia::protocols::xray::defaultFingerprint);
        fingerprints.removeDuplicates();
        for (const Carrier &carrier : bases) {
            for (const QString &fp : fingerprints) {
                for (const QString &alpn : XrayConfigModel::alpnOptions()) {
                    b.add(g, carrier, QStringLiteral("fingerprint=%1 alpn=%2").arg(fp, alpn), [fp, alpn](XrayConfigModel &m) {
                        m.setFingerprint(fp);
                        m.setAlpn(alpn);
                    });
                }
            }
        }
    }

    inline void tlsGroup(Builder &b)
    {
        const QString g = QStringLiteral("tls");
        tlsFingerprintGrid(b, g,
                           { raw("tls", ""), raw("tls", "xtls-rprx-vision"), raw("tls", "xtls-rprx-vision-udp443"),
                             xhttp("tls", "Stream-one") });
        b.text(g, { mkcp() }, QStringLiteral("alpn"), &XrayConfigModel::setAlpn, XrayConfigModel::alpnOptions());
        b.text(g, { raw("tls", "") }, QStringLiteral("alpn"), &XrayConfigModel::setAlpn,
               { "HTTP/2", "h2, http/1.1", "http/1.1,h2", "h3", "foo", "" });
        b.text(g, { raw("tls", "") }, QStringLiteral("fingerprint"), &XrayConfigModel::setFingerprint,
               { "", "foo", " Chrome " });
    }

    inline void tlsXhttpGroup(Builder &b)
    {
        tlsFingerprintGrid(b, QStringLiteral("tlsxhttp"),
                           { xhttp("tls", "Auto"), xhttp("tls", "Packet-up"), xhttp("tls", "Stream-up") });
    }

    inline void realityGroup(Builder &b)
    {
        const QString g = QStringLiteral("reality");
        QStringList fingerprints = XrayConfigModel::realityFingerprintOptions();
        for (const xraytest::RealityFingerprintCase &fp : xraytest::realityFingerprintCases()) {
            if (fp.hello == xraytest::RealityHello::Connects)
                fingerprints << fp.name;
        }
        fingerprints.removeDuplicates();
        b.text(g, carriers(QStringLiteral("reality")), QStringLiteral("fingerprint"), &XrayConfigModel::setFingerprint,
               fingerprints);
        b.text(g, { raw("reality", "xtls-rprx-vision") }, QStringLiteral("fingerprint"),
               &XrayConfigModel::setFingerprint, xraytest::legacyRealityFingerprints());
    }

    inline void oldClientGroup(Builder &b)
    {
        const QString g = QStringLiteral("oldclient");
        for (const Carrier &carrier : { raw("reality", "xtls-rprx-vision"), xhttp("reality", "Auto") }) {
            for (const xraytest::RealityFingerprintCase &fp : xraytest::realityFingerprintCases()) {
                const QString name = fp.name;
                b.add(g, carrier, QStringLiteral("client fingerprint=%1").arg(name), {},
                      [name](QJsonObject &client) { xraytest::setClientRealityFingerprint(client, name); });
            }
        }
    }

    inline void sniGroup(Builder &b)
    {
        const QString g = QStringLiteral("sni");
        const QList<Carrier> bases = { raw("tls", ""), xhttp("tls", "Stream-one"), mkcp(),
                                       raw("reality", "xtls-rprx-vision"), xhttp("reality", "Auto") };
        const QStringList values = { "www.googletagmanager.com",
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
        b.text(g, bases, QStringLiteral("sni"), &XrayConfigModel::setSni, values);
        const Carrier reality = raw("reality", "xtls-rprx-vision");
        b.add(g, reality, QStringLiteral("sni='www.microsoft.com' site='www.googletagmanager.com'"),
              [](XrayConfigModel &m) {
                  m.setSni(QStringLiteral("www.microsoft.com"));
                  m.setSite(QStringLiteral("www.googletagmanager.com"));
              });
        b.add(g, reality, QStringLiteral("sni='' site='www.microsoft.com'"), [](XrayConfigModel &m) {
            m.setSni(QString());
            m.setSite(QStringLiteral("www.microsoft.com"));
        });
    }

    inline void portGroup(Builder &b)
    {
        const QString g = QStringLiteral("port");
        b.text(g, { raw("reality", "xtls-rprx-vision"), mkcp() }, QStringLiteral("port"), &XrayConfigModel::setPort,
               { "2053", "65535", "1" });
        b.text(g, { raw("reality", "xtls-rprx-vision") }, QStringLiteral("port"), &XrayConfigModel::setPort,
               { "0", "65536", "-1", "", "abc" });
    }

    inline void xhttpGroup(Builder &b)
    {
        const QString g = QStringLiteral("xhttp");
        QList<Carrier> everyMode;
        for (const QString &security : XrayConfigModel::securityOptions()) {
            for (const QString &mode : XrayConfigModel::xhttpModeOptions())
                everyMode << xhttp(security, mode);
        }
        auto inMode = [](const QString &mode) {
            QList<Carrier> out;
            for (const QString &security : XrayConfigModel::securityOptions())
                out << xhttp(security, mode);
            return out;
        };
        const QList<Carrier> streamOne = inMode(QStringLiteral("Stream-one"));
        const QList<Carrier> packetUp = inMode(QStringLiteral("Packet-up"));
        const QList<Carrier> streamUp = inMode(QStringLiteral("Stream-up"));
        const QList<Range> xmuxRanges = { { "0", "0" }, { "1", "1" }, { "16", "32" }, { "0", "4" },
                                          { "4", "0" }, { "32", "16" }, { "abc", "5" } };
        auto placement = [](TextSetter setter, const QString &value) {
            return [setter, value](XrayConfigModel &m) { (m.*setter)(value); };
        };

        b.text(g, streamOne, QStringLiteral("mode"), &XrayConfigModel::setXhttpMode, { "", "packet-up", "foo" });
        b.text(g, streamOne, QStringLiteral("host"), &XrayConfigModel::setXhttpHost,
               { "", "www.googletagmanager.com", "example.com", "1.2.3.4", "::1", "a,b", "123", "bad host" });
        b.text(g, streamOne, QStringLiteral("path"), &XrayConfigModel::setXhttpPath,
               { "", "/", "/abc", "abc", " /abc ", "/a/b", "/x?y=1", "/%41", "/a b" });

        QStringList methods = XrayConfigModel::xhttpUplinkMethodOptions();
        methods << "post" << "GET" << "OPTIONS" << "FOO";
        b.text(g, everyMode, QStringLiteral("uplinkMethod"), &XrayConfigModel::setXhttpUplinkMethod, methods);

        for (const Carrier &carrier : everyMode) {
            for (const bool grpc : { true, false }) {
                for (const bool sse : { true, false }) {
                    const QString yes = QStringLiteral("true");
                    const QString no = QStringLiteral("false");
                    b.add(g, carrier, QStringLiteral("disableGrpc=%1 disableSse=%2").arg(grpc ? yes : no, sse ? yes : no),
                          [grpc, sse](XrayConfigModel &m) {
                              m.setXhttpDisableGrpc(grpc);
                              m.setXhttpDisableSse(sse);
                          });
                }
            }
        }

        QStringList sessionPlacements = XrayConfigModel::xhttpSessionPlacementOptions();
        sessionPlacements << "" << "Body" << "foo";
        b.text(g, everyMode, QStringLiteral("sessionPlacement"), &XrayConfigModel::setXhttpSessionPlacement,
               sessionPlacements);
        QStringList seqPlacements = XrayConfigModel::xhttpSeqPlacementOptions();
        seqPlacements << "" << "Body" << "foo";
        b.text(g, everyMode, QStringLiteral("seqPlacement"), &XrayConfigModel::setXhttpSeqPlacement, seqPlacements);
        const QStringList keys = { "", "sid", "X-Sid", "a-b_1", "a b", "a:b" };
        b.text(g, packetUp, QStringLiteral("sessionPlacement=Header sessionKey"), &XrayConfigModel::setXhttpSessionKey,
               keys, placement(&XrayConfigModel::setXhttpSessionPlacement, QStringLiteral("Header")));
        b.text(g, packetUp, QStringLiteral("seqPlacement=Header seqKey"), &XrayConfigModel::setXhttpSeqKey, keys,
               placement(&XrayConfigModel::setXhttpSeqPlacement, QStringLiteral("Header")));

        QStringList dataPlacements = XrayConfigModel::xhttpUplinkDataPlacementOptions();
        dataPlacements << "" << "foo";
        b.text(g, everyMode, QStringLiteral("uplinkDataPlacement"), &XrayConfigModel::setXhttpUplinkDataPlacement,
               dataPlacements);
        const Edit headerData = placement(&XrayConfigModel::setXhttpUplinkDataPlacement, QStringLiteral("Header"));
        b.text(g, packetUp, QStringLiteral("uplinkDataPlacement=Header uplinkDataKey"),
               &XrayConfigModel::setXhttpUplinkDataKey, { "", "xd", "X-Data", "a:b" }, headerData);
        b.text(g, packetUp, QStringLiteral("uplinkDataPlacement=Header uplinkChunkSize"),
               &XrayConfigModel::setXhttpUplinkChunkSize, { "", "0", "1", "63", "64", "4096", "65536", "abc" },
               headerData);

        b.text(g, packetUp, QStringLiteral("scMaxBufferedPosts"), &XrayConfigModel::setXhttpScMaxBufferedPosts,
               { "", "0", "1", "30", "1000", "-1", "abc" });
        b.range(g, packetUp, QStringLiteral("scMaxEachPostBytes"), &XrayConfigModel::setXhttpScMaxEachPostBytesMin,
                &XrayConfigModel::setXhttpScMaxEachPostBytesMax,
                { { "1000000", "1000000" }, { "100", "100" }, { "1", "1" }, { "0", "0" }, { "0", "1000" },
                  { "1000000", "0" }, { "2000", "1000" }, { "", "5000" }, { "abc", "5" } });
        b.range(g, packetUp, QStringLiteral("scMinPostsIntervalMs"), &XrayConfigModel::setXhttpScMinPostsIntervalMsMin,
                &XrayConfigModel::setXhttpScMinPostsIntervalMsMax,
                { { "100", "800" }, { "0", "0" }, { "0", "500" }, { "10", "10" }, { "800", "100" }, { "abc", "5" } });
        b.range(g, streamUp, QStringLiteral("scStreamUpServerSecs"), &XrayConfigModel::setXhttpScStreamUpServerSecsMin,
                &XrayConfigModel::setXhttpScStreamUpServerSecsMax,
                { { "1", "100" }, { "0", "0" }, { "0", "5" }, { "1", "0" }, { "5", "5" }, { "100", "1" } });

        for (const Carrier &carrier : streamOne) {
            b.add(g, carrier, QStringLiteral("xPaddingObfsMode=false"),
                  [](XrayConfigModel &m) { m.setXPaddingObfsMode(false); });
        }
        b.range(g, streamOne, QStringLiteral("xPaddingBytes"), &XrayConfigModel::setXPaddingBytesMin,
                &XrayConfigModel::setXPaddingBytesMax,
                { { "", "" }, { "100", "1000" }, { "0", "0" }, { "0", "1" }, { "0", "1000" }, { "1", "1" },
                  { "500", "100" }, { "100", "" }, { "-1", "5" } });
        b.text(g, streamOne, QStringLiteral("xPaddingKey"), &XrayConfigModel::setXPaddingKey, { "", "pad", "X-Pad", "a b" });
        b.text(g, streamOne, QStringLiteral("xPaddingPlacement=Header xPaddingHeader"), &XrayConfigModel::setXPaddingHeader,
               { "", "pad", "X-Pad", "a b" }, placement(&XrayConfigModel::setXPaddingPlacement, QStringLiteral("Header")));
        for (const QString &paddingPlacement : XrayConfigModel::xPaddingPlacementOptions()) {
            b.text(g, streamOne, QStringLiteral("xPaddingPlacement=%1 xPaddingMethod").arg(paddingPlacement),
                   &XrayConfigModel::setXPaddingMethod, XrayConfigModel::xPaddingMethodOptions(),
                   placement(&XrayConfigModel::setXPaddingPlacement, paddingPlacement));
        }
        b.text(g, streamOne, QStringLiteral("xPaddingPlacement"), &XrayConfigModel::setXPaddingPlacement,
               { "queryInHeader", "Body", "foo" });
        b.text(g, streamOne, QStringLiteral("xPaddingMethod"), &XrayConfigModel::setXPaddingMethod, { "Random", "foo" });

        for (const Carrier &carrier : streamOne) {
            b.add(g, carrier, QStringLiteral("xmuxEnabled=false"), [](XrayConfigModel &m) { m.setXmuxEnabled(false); });
            b.add(g, carrier, QStringLiteral("xmux maxConcurrency='1'..'1' maxConnections='1'..'1'"), [](XrayConfigModel &m) {
                m.setXmuxEnabled(true);
                m.setXmuxMaxConcurrencyMin(QStringLiteral("1"));
                m.setXmuxMaxConcurrencyMax(QStringLiteral("1"));
                m.setXmuxMaxConnectionsMin(QStringLiteral("1"));
                m.setXmuxMaxConnectionsMax(QStringLiteral("1"));
            });
        }
        const Edit xmuxOn = [](XrayConfigModel &m) { m.setXmuxEnabled(true); };
        b.range(g, streamOne, QStringLiteral("xmux maxConcurrency"), &XrayConfigModel::setXmuxMaxConcurrencyMin,
                &XrayConfigModel::setXmuxMaxConcurrencyMax, xmuxRanges, xmuxOn);
        b.range(g, streamOne, QStringLiteral("xmux maxConnections"), &XrayConfigModel::setXmuxMaxConnectionsMin,
                &XrayConfigModel::setXmuxMaxConnectionsMax, xmuxRanges, xmuxOn);
        b.range(g, streamOne, QStringLiteral("xmux cMaxReuseTimes"), &XrayConfigModel::setXmuxCMaxReuseTimesMin,
                &XrayConfigModel::setXmuxCMaxReuseTimesMax, xmuxRanges, xmuxOn);
        b.range(g, streamOne, QStringLiteral("xmux hMaxRequestTimes"), &XrayConfigModel::setXmuxHMaxRequestTimesMin,
                &XrayConfigModel::setXmuxHMaxRequestTimesMax, xmuxRanges, xmuxOn);
        b.range(g, streamOne, QStringLiteral("xmux hMaxReusableSecs"), &XrayConfigModel::setXmuxHMaxReusableSecsMin,
                &XrayConfigModel::setXmuxHMaxReusableSecsMax, xmuxRanges, xmuxOn);
        b.text(g, streamOne, QStringLiteral("xmux hKeepAlivePeriod"), &XrayConfigModel::setXmuxHKeepAlivePeriod,
               { "", "0", "30", "-1", "abc" }, xmuxOn);
    }

    inline void mkcpGroup(Builder &b)
    {
        const QString g = QStringLiteral("mkcp");
        const QList<Carrier> on = { mkcp() };
        b.text(g, on, QStringLiteral("tti"), &XrayConfigModel::setMkcpTti,
               { "", "50", "20", "10", "100", "9", "101", "0", "1000", "abc", "-1" });
        b.text(g, on, QStringLiteral("mtu"), &XrayConfigModel::setMkcpMtu,
               { "", "1350", "576", "1460", "575", "1461", "0", "21", "abc" });
        const QStringList capacities = { "", "5", "0", "1", "4095", "4096", "5000", "abc", "9999999999", "-1" };
        b.text(g, on, QStringLiteral("uplinkCapacity"), &XrayConfigModel::setMkcpUplinkCapacity, capacities);
        b.text(g, on, QStringLiteral("downlinkCapacity"), &XrayConfigModel::setMkcpDownlinkCapacity, capacities);
        b.text(g, on, QStringLiteral("cwndMultiplier"), &XrayConfigModel::setMkcpCwndMultiplier,
               { "", "1", "2", "5000", "5001", "6000", "0", "-5", "abc" });
        b.text(g, on, QStringLiteral("maxSendingWindow"), &XrayConfigModel::setMkcpMaxSendingWindow,
               { "", "1350", "1349", "0", "2097152", "abc", "-1" });
        b.add(g, mkcp(), QStringLiteral("mtu='1460' maxSendingWindow='1400'"), [](XrayConfigModel &m) {
            m.setMkcpMtu(QStringLiteral("1460"));
            m.setMkcpMaxSendingWindow(QStringLiteral("1400"));
        });
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

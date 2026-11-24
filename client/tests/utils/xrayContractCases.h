#ifndef XRAY_CONTRACT_CASES_H
#define XRAY_CONTRACT_CASES_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayInboundValidator.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "xrayMatrix.h"

namespace xraytest
{
    enum class AppVerdict {
        Ok,
        Blocked,
    };

    inline QString verdictName(AppVerdict v)
    {
        return v == AppVerdict::Ok ? QStringLiteral("ok") : QStringLiteral("blocked");
    }

    using InboundEdit = std::function<void(QJsonObject &)>;
    using ServerPrep = std::function<void(amnezia::XrayServerConfig &)>;

    struct ContractCase
    {
        QString name;
        QString ruleId;
        QString transport;
        QString security;
        QString flow;
        ServerPrep prep;
        InboundEdit edit;
        AppVerdict app = AppVerdict::Ok;
        QString coreReject;
        QString stricterWhy;

        ContractCase &rule(const QString &id)
        {
            ruleId = id;
            return *this;
        }
        ContractCase &blocked()
        {
            app = AppVerdict::Blocked;
            return *this;
        }
        ContractCase &reject(const QString &message)
        {
            app = AppVerdict::Blocked;
            coreReject = message;
            return *this;
        }
        ContractCase &stricter(const QString &why)
        {
            app = AppVerdict::Blocked;
            stricterWhy = why;
            return *this;
        }
    };

    struct ContractCtx
    {
        QString label;
        QString transport;
        QString security;
        QString flow;
        ServerPrep prep;
    };

    inline ContractCtx rawCtx(const QString &security, const QString &flow = QStringLiteral("xtls-rprx-vision"))
    {
        return { QStringLiteral("raw/%1/%2").arg(security, flow.isEmpty() ? QStringLiteral("noflow") : flow),
                 QStringLiteral("raw"),
                 security,
                 flow,
                 {} };
    }

    inline ContractCtx xhttpCtx(const QString &security, const QString &mode)
    {
        return { QStringLiteral("xhttp/%1/%2").arg(security, mode.toLower()), QStringLiteral("xhttp"), security,
                 QString(), [mode](amnezia::XrayServerConfig &srv) { srv.xhttp.mode = mode; } };
    }

    inline ContractCtx xhttpHeaderKeysCtx()
    {
        return { QStringLiteral("xhttp/tls/packet-up/header-placements"), QStringLiteral("xhttp"),
                 QStringLiteral("tls"), QString(), [](amnezia::XrayServerConfig &srv) {
                     srv.xhttp.mode = QStringLiteral("Packet-up");
                     srv.xhttp.sessionPlacement = QStringLiteral("Header");
                     srv.xhttp.seqPlacement = QStringLiteral("Header");
                     srv.xhttp.uplinkDataPlacement = QStringLiteral("Header");
                     srv.xhttp.xPadding.obfsMode = true;
                     srv.xhttp.xPadding.placement = QStringLiteral("Header");
                 } };
    }

    inline ContractCtx kcpCtx(const QString &security)
    {
        return { QStringLiteral("mkcp/%1").arg(security), QStringLiteral("mkcp"), security, QString(), {} };
    }

    inline ContractCase kase(const ContractCtx &ctx, const QString &what, const InboundEdit &edit)
    {
        ContractCase c;
        c.name = ctx.label + QLatin1Char('/') + what;
        c.transport = ctx.transport;
        c.security = ctx.security;
        c.flow = ctx.flow;
        c.prep = ctx.prep;
        c.edit = edit;
        if (ctx.security == QLatin1String("none"))
            c.reject(QStringLiteral("vless without TLS"));
        return c;
    }

    inline void putPath(QJsonObject &object, const QStringList &path, const QJsonValue &value)
    {
        const QString key = path.first();
        if (path.size() == 1) {
            if (value.isUndefined())
                object.remove(key);
            else
                object[key] = value;
            return;
        }
        QJsonObject child = object.value(key).toObject();
        putPath(child, path.mid(1), value);
        object[key] = child;
    }

    inline InboundEdit put(const QStringList &path, const QJsonValue &value)
    {
        return [path, value](QJsonObject &inbound) { putPath(inbound, path, value); };
    }

    inline InboundEdit xhttpValue(const QString &key, const QJsonValue &value)
    {
        return put({ QStringLiteral("streamSettings"), QStringLiteral("xhttpSettings"), key }, value);
    }

    inline InboundEdit xmuxValue(const QString &key, const QJsonValue &value)
    {
        return put({ QStringLiteral("streamSettings"), QStringLiteral("xhttpSettings"), QStringLiteral("xmux"), key },
                   value);
    }

    inline InboundEdit kcpValue(const QString &key, const QJsonValue &value)
    {
        return put({ QStringLiteral("streamSettings"), QStringLiteral("kcpSettings"), key }, value);
    }

    inline InboundEdit tlsValue(const QString &key, const QJsonValue &value)
    {
        return put({ QStringLiteral("streamSettings"), QStringLiteral("tlsSettings"), key }, value);
    }

    inline InboundEdit streamValue(const QString &key, const QJsonValue &value)
    {
        return put({ QStringLiteral("streamSettings"), key }, value);
    }

    inline InboundEdit realityServerName(const QString &name)
    {
        return put({ QStringLiteral("streamSettings"), QStringLiteral("realitySettings"), QStringLiteral("serverNames") },
                   QJsonArray { name });
    }

    inline InboundEdit clientFlow(const QString &flow)
    {
        return [flow](QJsonObject &inbound) {
            QJsonObject settings = inbound.value(QStringLiteral("settings")).toObject();
            QJsonArray clients = settings.value(QStringLiteral("clients")).toArray();
            QJsonObject client = clients.first().toObject();
            if (flow.isEmpty())
                client.remove(QStringLiteral("flow"));
            else
                client[QStringLiteral("flow")] = flow;
            clients[0] = client;
            settings[QStringLiteral("clients")] = clients;
            inbound[QStringLiteral("settings")] = settings;
        };
    }

    inline InboundEdit both(const InboundEdit &first, const InboundEdit &second)
    {
        return [first, second](QJsonObject &inbound) {
            first(inbound);
            second(inbound);
        };
    }

    inline QJsonValue rangeValue(const QString &min, const QString &max)
    {
        if (min.isEmpty() && max.isEmpty())
            return QJsonValue(QJsonValue::Undefined);
        return QStringLiteral("%1-%2").arg(min, max);
    }

    inline QString shown(const QString &value)
    {
        if (value.isEmpty())
            return QStringLiteral("empty");
        if (value.size() > 24)
            return QStringLiteral("%1chars").arg(value.size());
        return value;
    }

    inline amnezia::XrayServerConfig contractServerConfig(const ContractCase &c)
    {
        amnezia::XrayServerConfig srv = baseConfig(c.transport, c.security, c.flow);
        if (c.prep)
            c.prep(srv);
        return srv;
    }

    struct ContractResult
    {
        QString templateText;
        QStringList errors;
        amnezia::XrayServerConfig srv;
        bool applied = false;
    };

    inline ContractResult runContract(const ContractCase &c)
    {
        ContractResult result;
        result.srv = contractServerConfig(c);
        QJsonObject inbound = amnezia::xrayInbound::parse(amnezia::xrayInbound::templateFromServerConfig(result.srv));
        if (c.edit)
            c.edit(inbound);
        result.templateText = amnezia::xrayInbound::format(inbound);

        for (const amnezia::XrayInboundIssue &issue : amnezia::validateXrayInbound(result.templateText).issues) {
            if (issue.level == amnezia::XrayInboundIssue::Level::Error)
                result.errors << QStringLiteral("line %1: %2").arg(issue.line).arg(issue.message);
        }
        result.applied = amnezia::xrayInbound::applyTemplate(result.templateText, result.srv);
        return result;
    }

    inline amnezia::xrayInbound::Secrets contractSecrets()
    {
        amnezia::xrayInbound::Secrets secrets;
        secrets.clients = QJsonArray { amnezia::XrayServerConfig::makeClientEntry(clientId(), QString()) };
        secrets.realityPrivateKey = key32('\x01');
        secrets.realityShortId = shortId();
        return secrets;
    }

    struct RangeExpect
    {
        AppVerdict app;
        const char *coreReject;
    };

    inline const QList<QPair<QString, QString>> &rangeInputs()
    {
        static const QList<QPair<QString, QString>> inputs = {
            { "", "" },       { "0", "0" },  { "0", "100" }, { "100", "0" },        { "1", "1" },
            { "400", "200" }, { "-1", "5" }, { "abc", "5" }, { "1", "2147483648" }, { "1", "9223372036854775808" },
        };
        return inputs;
    }

    inline void addRange(QList<ContractCase> &out, const ContractCtx &ctx, const QString &field, const QString &ruleId,
                         const std::function<InboundEdit(const QJsonValue &)> &setter, const RangeExpect (&expect)[10])
    {
        for (int i = 0; i < rangeInputs().size(); ++i) {
            const QString min = rangeInputs().at(i).first;
            const QString max = rangeInputs().at(i).second;
            const QString what = min.isEmpty() && max.isEmpty() ? QStringLiteral("%1=empty").arg(field)
                                                                : QStringLiteral("%1=%2-%3").arg(field, min, max);
            ContractCase c = kase(ctx, what, setter(rangeValue(min, max))).rule(ruleId);
            c.app = expect[i].app;
            c.coreReject = QString::fromLatin1(expect[i].coreReject);
            out.append(c);
        }
    }

    inline QList<ContractCase> buildContractCases()
    {
        using A = AppVerdict;
        QList<ContractCase> out;
        const QJsonValue removed(QJsonValue::Undefined);

        const ContractCtx rawReality = rawCtx(QStringLiteral("reality"));
        const ContractCtx rawTls = rawCtx(QStringLiteral("tls"));
        const ContractCtx rawTlsNoFlow = rawCtx(QStringLiteral("tls"), QString());
        const ContractCtx rawRealityNoFlow = rawCtx(QStringLiteral("reality"), QString());
        const ContractCtx rawNone = rawCtx(QStringLiteral("none"), QString());
        const ContractCtx xTlsOne = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Stream-one"));
        const ContractCtx xTlsPacket = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Packet-up"));
        const ContractCtx xTlsUp = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Stream-up"));
        const ContractCtx xRealityOne = xhttpCtx(QStringLiteral("reality"), QStringLiteral("Stream-one"));
        const ContractCtx kcpTls = kcpCtx(QStringLiteral("tls"));

        for (const ContractCtx &ctx : { rawReality, rawTls, rawTlsNoFlow, rawNone, xTlsOne, xTlsPacket, xTlsUp,
                                        xRealityOne, kcpTls, xhttpHeaderKeysCtx() })
            out.append(kase(ctx, QStringLiteral("baseline"), {}));

        for (const int port : { 1, 65535 })
            out.append(kase(rawReality, QStringLiteral("port=%1").arg(port), put({ QStringLiteral("port") }, port)));
        for (const int port : { 0, 65536, -1 })
            out.append(
                    kase(rawReality, QStringLiteral("port=%1").arg(port), put({ QStringLiteral("port") }, port)).blocked());
        out.append(kase(rawReality, QStringLiteral("port=abc"), put({ QStringLiteral("port") }, QStringLiteral("abc")))
                           .blocked());
        out.append(kase(rawReality, QStringLiteral("port=missing"), put({ QStringLiteral("port") }, removed)).blocked());

        const QString anyServerName = QStringLiteral("the core takes any serverName; the TLS handshake fails later");
        const QStringList badNames = { QStringLiteral("*"),
                                       QStringLiteral("*.example.com"),
                                       QStringLiteral("a b"),
                                       QStringLiteral("пример.рф"),
                                       QStringLiteral("example.com."),
                                       QStringLiteral("12345"),
                                       QString(250, QLatin1Char('a')) + QStringLiteral(".com") };
        const QStringList goodNames = { QStringLiteral("www.example.com"), QStringLiteral("xn--e1afmkfd.com"),
                                        QStringLiteral("xn--e1afmkfd.xn--p1ai") };
        for (const QString &name : goodNames) {
            out.append(kase(rawTls, QStringLiteral("serverName=") + name, tlsValue(QStringLiteral("serverName"), name)));
            out.append(kase(rawReality, QStringLiteral("serverNames=") + name, realityServerName(name)));
        }
        for (const QString &name : badNames) {
            out.append(kase(rawTls, QStringLiteral("serverName=") + shown(name),
                            tlsValue(QStringLiteral("serverName"), name))
                               .stricter(anyServerName));
            out.append(kase(rawReality, QStringLiteral("serverNames=") + shown(name), realityServerName(name))
                               .stricter(anyServerName));
        }
        out.append(kase(rawTls, QStringLiteral("serverName=empty"), tlsValue(QStringLiteral("serverName"), removed)));
        for (const QString ip : { QStringLiteral("203.0.113.10"), QStringLiteral("2001:db8::1") }) {
            out.append(kase(rawTls, QStringLiteral("serverName=") + ip, tlsValue(QStringLiteral("serverName"), ip)));
            out.append(kase(rawReality, QStringLiteral("serverNames=") + ip, realityServerName(ip))
                               .rule(QStringLiteral("reality-sni-ip"))
                               .stricter(QStringLiteral("REALITY with an IP SNI builds but carries no traffic")));
        }
        out.append(kase(rawReality, QStringLiteral("serverNames=empty"), realityServerName(QString()))
                           .stricter(anyServerName));

        for (const ContractCtx &ctx : { rawTls, xTlsOne }) {
            const QList<QPair<QString, QJsonValue>> goodAlpn = {
                { QStringLiteral("h2"), QJsonArray { QStringLiteral("h2") } },
                { QStringLiteral("http/1.1"), QJsonArray { QStringLiteral("http/1.1") } },
                { QStringLiteral("h2,http/1.1"), QJsonArray { QStringLiteral("h2"), QStringLiteral("http/1.1") } },
                { QStringLiteral("empty"), removed },
            };
            for (const auto &alpn : goodAlpn) {
                ContractCase c =
                        kase(ctx, QStringLiteral("alpn=") + alpn.first, tlsValue(QStringLiteral("alpn"), alpn.second));
                if (ctx.transport == QLatin1String("xhttp"))
                    c.rule(QStringLiteral("xhttp-http-version"));
                out.append(c);
            }
            for (const QString alpn : { QStringLiteral("HTTP/2"), QStringLiteral("foo") })
                out.append(
                        kase(ctx, QStringLiteral("alpn=") + alpn, tlsValue(QStringLiteral("alpn"), QJsonArray { alpn }))
                                .stricter(QStringLiteral("the core takes any ALPN token; the handshake fails later")));
        }

        for (const QString host : { QStringLiteral("cdn.example.com"), QStringLiteral("203.0.113.10") })
            out.append(kase(xTlsOne, QStringLiteral("host=") + host, xhttpValue(QStringLiteral("host"), host)));
        out.append(kase(xTlsOne, QStringLiteral("host=empty"), xhttpValue(QStringLiteral("host"), removed)));
        for (const QString host :
             { QStringLiteral("bad host"), QStringLiteral("a:80"), QStringLiteral("пример.рф"), QStringLiteral("::1") })
            out.append(kase(xTlsOne, QStringLiteral("host=") + host, xhttpValue(QStringLiteral("host"), host))
                               .stricter(QStringLiteral("the core takes any host string")));

        for (const QString path :
             { QStringLiteral("/"), QStringLiteral("text"), QStringLiteral("/?ed=1"), QStringLiteral("/%"),
               QStringLiteral("/a b"), QStringLiteral("/") + QString(2000, QLatin1Char('p')) })
            out.append(kase(xTlsOne, QStringLiteral("path=") + shown(path), xhttpValue(QStringLiteral("path"), path)));
        out.append(kase(xTlsOne, QStringLiteral("path=empty"), xhttpValue(QStringLiteral("path"), removed)));

        const QStringList modes = { QStringLiteral("Auto"), QStringLiteral("Packet-up"), QStringLiteral("Stream-up"),
                                    QStringLiteral("Stream-one") };
        for (const QString security : { QStringLiteral("none"), QStringLiteral("tls"), QStringLiteral("reality") }) {
            for (const QString &mode : modes) {
                QString ruleId = QStringLiteral("xhttp-mode-enum");
                if (mode == QLatin1String("Auto"))
                    ruleId = security == QLatin1String("reality") ? QStringLiteral("xhttp-auto-resolve-reality")
                                                                  : QStringLiteral("xhttp-auto-resolve-other");
                out.append(kase(xhttpCtx(security, mode), QStringLiteral("mode"), {}).rule(ruleId));
            }
        }
        for (const QString mode : { QStringLiteral("foo"), QStringLiteral("Stream-one") })
            out.append(kase(xTlsOne, QStringLiteral("mode=") + mode, xhttpValue(QStringLiteral("mode"), mode))
                               .rule(QStringLiteral("xhttp-mode-enum"))
                               .reject(QStringLiteral("unsupported mode")));
        out.append(kase(xTlsOne, QStringLiteral("mode=empty"), xhttpValue(QStringLiteral("mode"), removed)));

        for (const QString &mode : modes) {
            const ContractCtx ctx = xhttpCtx(QStringLiteral("tls"), mode);
            const bool packetUp = mode == QLatin1String("Packet-up");
            for (const QString &method : xhttpUplinkMethodOptions())
                out.append(kase(ctx, QStringLiteral("method=") + method,
                                xhttpValue(QStringLiteral("uplinkHTTPMethod"), method)));
            ContractCase get = kase(ctx, QStringLiteral("method=GET"),
                                    xhttpValue(QStringLiteral("uplinkHTTPMethod"), QStringLiteral("GET")))
                                       .rule(QStringLiteral("xhttp-get-packet-up"));
            if (!packetUp && mode != QLatin1String("Auto"))
                get.reject(QStringLiteral("GET only in packet-up"));
            else if (!packetUp)
                get.blocked();
            out.append(get);

            for (const QString placement : { QStringLiteral("path"), QStringLiteral("header"), QStringLiteral("cookie"),
                                             QStringLiteral("query") }) {
                out.append(kase(ctx, QStringLiteral("sessionIDPlacement=") + placement,
                                xhttpValue(QStringLiteral("sessionIDPlacement"), placement))
                                   .rule(QStringLiteral("xhttp-placement-enum")));
                out.append(kase(ctx, QStringLiteral("seqPlacement=") + placement,
                                xhttpValue(QStringLiteral("seqPlacement"), placement))
                                   .rule(QStringLiteral("xhttp-placement-enum")));
            }
            for (const QString placement : { QStringLiteral("body"), QStringLiteral("auto"), QStringLiteral("header"),
                                             QStringLiteral("cookie") }) {
                ContractCase c = kase(ctx, QStringLiteral("uplinkDataPlacement=") + placement,
                                      xhttpValue(QStringLiteral("uplinkDataPlacement"), placement));
                const bool packetUpOnly = placement == QLatin1String("header") || placement == QLatin1String("cookie");
                if (packetUpOnly && !packetUp) {
                    c.rule(QStringLiteral("xhttp-uplink-data-placement-packet-up"));
                    if (mode == QLatin1String("Auto"))
                        c.blocked();
                    else
                        c.reject(QStringLiteral("only in packet-up"));
                }
                out.append(c);
            }
        }
        out.append(kase(xTlsPacket, QStringLiteral("method=get"),
                        xhttpValue(QStringLiteral("uplinkHTTPMethod"), QStringLiteral("get"))));
        out.append(kase(xTlsPacket, QStringLiteral("method=foo"),
                        xhttpValue(QStringLiteral("uplinkHTTPMethod"), QStringLiteral("foo")))
                           .stricter(QStringLiteral("the core takes any HTTP method token")));
        for (const QString placement : { QStringLiteral("foo"), QStringLiteral("none"), QStringLiteral("Path") }) {
            out.append(kase(xTlsPacket, QStringLiteral("sessionIDPlacement=") + placement,
                            xhttpValue(QStringLiteral("sessionIDPlacement"), placement))
                               .rule(QStringLiteral("xhttp-placement-enum"))
                               .reject(QStringLiteral("unsupported session placement")));
            out.append(kase(xTlsPacket, QStringLiteral("seqPlacement=") + placement,
                            xhttpValue(QStringLiteral("seqPlacement"), placement))
                               .rule(QStringLiteral("xhttp-placement-enum"))
                               .reject(QStringLiteral("unsupported seq placement")));
        }
        for (const QString placement : { QStringLiteral("foo"), QStringLiteral("Body") })
            out.append(kase(xTlsPacket, QStringLiteral("uplinkDataPlacement=") + placement,
                            xhttpValue(QStringLiteral("uplinkDataPlacement"), placement))
                               .reject(QStringLiteral("unsupported uplink data placement")));

        const ContractCtx keysCtx = xhttpHeaderKeysCtx();
        for (const QString field :
             { QStringLiteral("sessionIDKey"), QStringLiteral("seqKey"), QStringLiteral("uplinkDataKey"),
               QStringLiteral("xPaddingKey"), QStringLiteral("xPaddingHeader") }) {
            for (const QString key :
                 { QString(), QStringLiteral("x_ok"), QStringLiteral("X-Header"), QString(300, QLatin1Char('k')) })
                out.append(kase(keysCtx, field + QLatin1Char('=') + shown(key), xhttpValue(field, key)));
            for (const QString key :
                 { QStringLiteral("a b"), QStringLiteral("ключ"), QStringLiteral("a:b"), QStringLiteral("a;b") })
                out.append(kase(keysCtx, field + QLatin1Char('=') + key, xhttpValue(field, key))
                                   .stricter(QStringLiteral(
                                           "the core takes the name, but the request carries no traffic")));
        }

        const char *noRange = "Invalid integer range";
        const char *disabled = "cannot be disabled";
        const RangeExpect xPaddingBytes[10] = {
            { A::Ok, "" },
            { A::Ok, "" },
            { A::Blocked, disabled },
            { A::Blocked, disabled },
            { A::Ok, "" },
            { A::Ok, "" },
            { A::Blocked, disabled },
            { A::Blocked, noRange },
            { A::Blocked, disabled },
            { A::Blocked, noRange },
        };
        addRange(
                out, xTlsOne, QStringLiteral("xPaddingBytes"), QStringLiteral("xhttp-padding-bytes"),
                [](const QJsonValue &v) { return xhttpValue(QStringLiteral("xPaddingBytes"), v); }, xPaddingBytes);

        const RangeExpect scMaxEachPostBytes[10] = {
            { A::Ok, "" },      { A::Ok, "" },           { A::Blocked, "" }, { A::Blocked, "" },
            { A::Blocked, "" }, { A::Blocked, "" },      { A::Blocked, "" }, { A::Blocked, noRange },
            { A::Blocked, "" }, { A::Blocked, noRange },
        };
        addRange(
                out, xTlsPacket, QStringLiteral("scMaxEachPostBytes"), QStringLiteral("xhttp-sc-max-each-post-bytes"),
                [](const QJsonValue &v) { return xhttpValue(QStringLiteral("scMaxEachPostBytes"), v); },
                scMaxEachPostBytes);

        const RangeExpect signedRange[10] = {
            { A::Ok, "" }, { A::Ok, "" }, { A::Ok, "" },           { A::Ok, "" },      { A::Ok, "" },
            { A::Ok, "" }, { A::Ok, "" }, { A::Blocked, noRange }, { A::Blocked, "" }, { A::Blocked, noRange },
        };
        addRange(
                out, xTlsPacket, QStringLiteral("scMinPostsIntervalMs"), QStringLiteral("xhttp-min-posts-interval-zero"),
                [](const QJsonValue &v) { return xhttpValue(QStringLiteral("scMinPostsIntervalMs"), v); }, signedRange);

        const RangeExpect scStreamUpServerSecs[10] = {
            { A::Ok, "" }, { A::Ok, "" }, { A::Blocked, "" },      { A::Blocked, "" }, { A::Ok, "" },
            { A::Ok, "" }, { A::Ok, "" }, { A::Blocked, noRange }, { A::Blocked, "" }, { A::Blocked, noRange },
        };
        addRange(
                out, xTlsUp, QStringLiteral("scStreamUpServerSecs"), QStringLiteral("xhttp-stream-up-server-secs-zero"),
                [](const QJsonValue &v) { return xhttpValue(QStringLiteral("scStreamUpServerSecs"), v); },
                scStreamUpServerSecs);

        for (const QString field :
             { QStringLiteral("maxConcurrency"), QStringLiteral("maxConnections"), QStringLiteral("cMaxReuseTimes"),
               QStringLiteral("hMaxRequestTimes"), QStringLiteral("hMaxReusableSecs") })
            addRange(
                    out, xTlsOne, QStringLiteral("xmux.") + field, QStringLiteral("xmux-zero-unlimited"),
                    [field](const QJsonValue &v) { return xmuxValue(field, v); }, signedRange);

        out.append(kase(xTlsOne, QStringLiteral("xmux=concurrency+connections"),
                        both(xmuxValue(QStringLiteral("maxConcurrency"), 1),
                             xmuxValue(QStringLiteral("maxConnections"), 1)))
                           .rule(QStringLiteral("xmux-exclusive"))
                           .reject(QStringLiteral("maxConnections cannot be specified together")));
        out.append(kase(xTlsOne, QStringLiteral("xmux=concurrency 0+connections 1"),
                        both(xmuxValue(QStringLiteral("maxConcurrency"), 0),
                             xmuxValue(QStringLiteral("maxConnections"), 1)))
                           .rule(QStringLiteral("xmux-zero-unlimited")));

        for (const QJsonValue v : { QJsonValue(0), QJsonValue(1), QJsonValue(65536), QJsonValue(-1) })
            out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=%1").arg(v.toInt()),
                            xhttpValue(QStringLiteral("uplinkChunkSize"), v)));
        out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=empty"),
                        xhttpValue(QStringLiteral("uplinkChunkSize"), removed)));
        out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=2147483648"),
                        xhttpValue(QStringLiteral("uplinkChunkSize"), QStringLiteral("2147483648")))
                           .blocked());
        out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=abc"),
                        xhttpValue(QStringLiteral("uplinkChunkSize"), QStringLiteral("abc")))
                           .reject(noRange));

        for (const int v : { 0, 1, 100000 })
            out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=%1").arg(v),
                            xhttpValue(QStringLiteral("scMaxBufferedPosts"), v))
                               .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")));
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=100001"),
                        xhttpValue(QStringLiteral("scMaxBufferedPosts"), 100001))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc"))
                           .stricter(QStringLiteral("the server preallocates a buffer per post")));
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=-1"),
                        xhttpValue(QStringLiteral("scMaxBufferedPosts"), -1))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc"))
                           .stricter(QStringLiteral("a negative value carries no traffic")));
        for (const QString v : { QStringLiteral("abc"), QStringLiteral("5") })
            out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=\"%1\"").arg(v),
                            xhttpValue(QStringLiteral("scMaxBufferedPosts"), v))
                               .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc"))
                               .reject(QStringLiteral("cannot unmarshal")));

        for (const int v : { 0, 30, -1 })
            out.append(kase(xTlsOne, QStringLiteral("hKeepAlivePeriod=%1").arg(v),
                            xmuxValue(QStringLiteral("hKeepAlivePeriod"), v)));
        for (const QString v : { QStringLiteral("abc"), QStringLiteral("30") })
            out.append(kase(xTlsOne, QStringLiteral("hKeepAlivePeriod=\"%1\"").arg(v),
                            xmuxValue(QStringLiteral("hKeepAlivePeriod"), v))
                               .reject(QStringLiteral("cannot unmarshal")));
        out.append(kase(xTlsOne, QStringLiteral("alpn=http/1.1+hKeepAlivePeriod=30"),
                        both(tlsValue(QStringLiteral("alpn"), QJsonArray { QStringLiteral("http/1.1") }),
                             xmuxValue(QStringLiteral("hKeepAlivePeriod"), 30)))
                           .rule(QStringLiteral("use-keepalive-h1-alpn")));

        struct KcpRow
        {
            QString field;
            QString ruleId;
            QList<QJsonValue> ok;
            QList<QJsonValue> stricter;
            QList<QPair<QJsonValue, QString>> rejected;
            QString why;
        };
        const QString unmarshal = QStringLiteral("cannot unmarshal");
        const QList<KcpRow> kcpRows = {
            { QStringLiteral("mtu"),
              QStringLiteral("kcp-mtu"),
              { removed, 576, 1460 },
              { 21, 575, 1461 },
              { { 20, QStringLiteral("MTU must be at least 21") }, { QStringLiteral("abc"), unmarshal }, { -1, unmarshal } },
              QStringLiteral("the app range 576-1460 is narrower than the core (>= 21)") },
            { QStringLiteral("tti"),
              QStringLiteral("kcp-tti"),
              { 10, 100 },
              { 101, 1000 },
              { { 9, QStringLiteral("TTI must be between") }, { 1001, QStringLiteral("TTI must be between") } },
              QStringLiteral("the app range 10-100 is narrower than the core (10-1000)") },
            { QStringLiteral("cwndMultiplier"),
              QStringLiteral("kcp-cwnd"),
              { 1, 5000 },
              { 5001 },
              { { 0, QStringLiteral("CwndMultiplier must be at least 1") } },
              QStringLiteral("large cwndMultiplier overflows the window at runtime") },
            { QStringLiteral("maxSendingWindow"),
              QStringLiteral("kcp-sending-window"),
              { removed, 1350, 2147483647 },
              {},
              { { 1349, QStringLiteral("MaxSendingWindow must be at least") }, { QStringLiteral("abc"), unmarshal } },
              QString() },
            { QStringLiteral("uplinkCapacity"),
              QStringLiteral("kcp-capacity-overflow"),
              { 0, 4095 },
              { 4096 },
              { { -1, unmarshal }, { QStringLiteral("abc"), unmarshal } },
              QStringLiteral("capacity above 4095 overflows at runtime") },
            { QStringLiteral("downlinkCapacity"),
              QStringLiteral("kcp-capacity-overflow"),
              { 0, 4095 },
              { 4096 },
              { { -1, unmarshal }, { QStringLiteral("abc"), unmarshal } },
              QStringLiteral("capacity above 4095 overflows at runtime") },
        };
        auto valueName = [](const QJsonValue &v) {
            if (v.isUndefined())
                return QStringLiteral("empty");
            return v.isString() ? QStringLiteral("\"%1\"").arg(v.toString()) : QString::number(v.toInteger());
        };
        for (const KcpRow &row : kcpRows) {
            for (const QJsonValue &v : row.ok)
                out.append(
                        kase(kcpTls, row.field + QLatin1Char('=') + valueName(v), kcpValue(row.field, v)).rule(row.ruleId));
            for (const QJsonValue &v : row.stricter)
                out.append(kase(kcpTls, row.field + QLatin1Char('=') + valueName(v), kcpValue(row.field, v))
                                   .rule(row.ruleId)
                                   .stricter(row.why));
            for (const auto &v : row.rejected)
                out.append(kase(kcpTls, row.field + QLatin1Char('=') + valueName(v.first), kcpValue(row.field, v.first))
                                   .rule(row.ruleId)
                                   .reject(v.second));
        }

        for (const QString placement : { QStringLiteral("cookie"), QStringLiteral("header"), QStringLiteral("query"),
                                         QStringLiteral("queryInHeader") }) {
            for (const QString method : { QStringLiteral("repeat-x"), QStringLiteral("tokenish") })
                out.append(kase(xTlsOne, QStringLiteral("xPadding=%1/%2").arg(placement, method),
                                both(both(xhttpValue(QStringLiteral("xPaddingObfsMode"), true),
                                          xhttpValue(QStringLiteral("xPaddingPlacement"), placement)),
                                     xhttpValue(QStringLiteral("xPaddingMethod"), method)))
                                   .rule(QStringLiteral("xhttp-padding-placement")));
        }
        for (const bool obfs : { true, false })
            out.append(kase(xTlsOne, QStringLiteral("obfs %1+xPaddingPlacement=foo").arg(obfs ? "on" : "off"),
                            both(xhttpValue(QStringLiteral("xPaddingObfsMode"), obfs),
                                 xhttpValue(QStringLiteral("xPaddingPlacement"), QStringLiteral("foo"))))
                               .rule(QStringLiteral("xhttp-padding-placement"))
                               .reject(QStringLiteral("unsupported padding placement")));
        for (const QString method : { QStringLiteral("foo"), QStringLiteral("Repeat-x") })
            out.append(kase(xTlsOne, QStringLiteral("xPaddingMethod=") + method,
                            xhttpValue(QStringLiteral("xPaddingMethod"), method))
                               .rule(QStringLiteral("xhttp-padding-method"))
                               .reject(QStringLiteral("unsupported padding method")));
        out.append(kase(xTlsOne, QStringLiteral("obfs off+xPaddingBytes=200-400"),
                        both(xhttpValue(QStringLiteral("xPaddingObfsMode"), false),
                             xhttpValue(QStringLiteral("xPaddingBytes"), QStringLiteral("200-400"))))
                           .rule(QStringLiteral("use-padding-bytes-without-obfs")));

        const QString vision = QStringLiteral("xtls-rprx-vision");
        for (const ContractCtx &ctx : { rawTlsNoFlow, rawRealityNoFlow }) {
            out.append(kase(ctx, QStringLiteral("flow=empty"), clientFlow(QString())));
            out.append(kase(ctx, QStringLiteral("flow=") + vision, clientFlow(vision)));
            out.append(kase(ctx, QStringLiteral("flow=xtls-rprx-vision-udp443"),
                            clientFlow(QStringLiteral("xtls-rprx-vision-udp443")))
                               .rule(QStringLiteral("vision-udp443-client-only"))
                               .reject(QStringLiteral("doesn't support")));
            out.append(kase(ctx, QStringLiteral("flow=foo"), clientFlow(QStringLiteral("foo")))
                               .reject(QStringLiteral("doesn't support")));
        }
        out.append(kase(rawNone, QStringLiteral("flow=vision"), clientFlow(vision))
                           .rule(QStringLiteral("vision-needs-security")));
        out.append(kase(xTlsOne, QStringLiteral("flow=vision"), clientFlow(vision))
                           .rule(QStringLiteral("vision-needs-raw"))
                           .blocked());
        out.append(kase(kcpTls, QStringLiteral("flow=vision"), clientFlow(vision))
                           .rule(QStringLiteral("vision-needs-raw"))
                           .blocked());

        out.append(kase(rawTls, QStringLiteral("security=foo"),
                        streamValue(QStringLiteral("security"), QStringLiteral("foo")))
                           .reject(QStringLiteral("Unknown security")));
        out.append(kase(rawRealityNoFlow, QStringLiteral("network=foo"),
                        streamValue(QStringLiteral("network"), QStringLiteral("foo")))
                           .reject(QStringLiteral("unknown transport protocol")));
        out.append(kase(rawRealityNoFlow, QStringLiteral("network=ws"),
                        streamValue(QStringLiteral("network"), QStringLiteral("ws")))
                           .rule(QStringLiteral("reality-network"))
                           .reject(QStringLiteral("REALITY only supports")));
        out.append(kase(rawTlsNoFlow, QStringLiteral("network=ws"),
                        streamValue(QStringLiteral("network"), QStringLiteral("ws")))
                           .rule(QStringLiteral("app-networks"))
                           .stricter(QStringLiteral("the app builds client configs only for raw, xhttp and kcp")));

        return out;
    }

    inline const QList<ContractCase> &contractCases()
    {
        static const QList<ContractCase> all = buildContractCases();
        return all;
    }
}

#endif

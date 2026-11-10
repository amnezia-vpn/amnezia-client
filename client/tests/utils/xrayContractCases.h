#ifndef XRAY_CONTRACT_CASES_H
#define XRAY_CONTRACT_CASES_H

#include <functional>
#include <tuple>

#include <QList>
#include <QString>
#include <QStringList>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayMatrix.h"

namespace xraytest
{
    enum class AppVerdict {
        Ok,
        Blocked,
    };

    enum class CoreVerdict {
        Accept,
        Reject,
        Explore,
    };

    inline QString verdictName(AppVerdict v)
    {
        return v == AppVerdict::Ok ? QStringLiteral("ok") : QStringLiteral("blocked");
    }

    inline QString verdictName(CoreVerdict v)
    {
        switch (v) {
        case CoreVerdict::Accept: return QStringLiteral("accept");
        case CoreVerdict::Reject: return QStringLiteral("reject");
        case CoreVerdict::Explore: return QStringLiteral("explore");
        }
        return {};
    }

    using ModelEdit = std::function<void(XrayConfigModel &)>;
    using Setter = void (XrayConfigModel::*)(const QString &);

    struct ContractCase
    {
        QString name;
        QString ruleId;
        QString transport;
        QString security;
        QString flow;
        ModelEdit edit;
        AppVerdict app = AppVerdict::Ok;
        CoreVerdict core = CoreVerdict::Accept;
        QString coreMessage;
        QString stricterWhy;
        QString knownIssue;
        bool viaUi = true;

        ContractCase &rule(const QString &id) { ruleId = id; return *this; }
        ContractCase &blocked() { app = AppVerdict::Blocked; return *this; }
        ContractCase &reject(const QString &message = QString()) { core = CoreVerdict::Reject; coreMessage = message; return *this; }
        ContractCase &explore() { core = CoreVerdict::Explore; return *this; }
        ContractCase &stricter(const QString &why) { stricterWhy = why; return *this; }
        ContractCase &known(const QString &why) { knownIssue = why; return *this; }
        ContractCase &rawInput() { viaUi = false; return *this; }
    };

    struct ContractCtx
    {
        QString label;
        QString transport;
        QString security;
        QString flow;
        ModelEdit prep;
    };

    inline ContractCtx rawCtx(const QString &security, const QString &flow = QStringLiteral("xtls-rprx-vision"))
    {
        return { QStringLiteral("raw/%1/%2").arg(security, flow.isEmpty() ? QStringLiteral("noflow") : flow),
                 QStringLiteral("raw"), security, flow, {} };
    }

    inline ContractCtx xhttpCtx(const QString &security, const QString &mode)
    {
        return { QStringLiteral("xhttp/%1/%2").arg(security, mode.toLower()), QStringLiteral("xhttp"), security,
                 QString(), [mode](XrayConfigModel &m) { m.setXhttpMode(mode); } };
    }

    inline ContractCtx xhttpHeaderKeysCtx()
    {
        return { QStringLiteral("xhttp/tls/packet-up/header-placements"), QStringLiteral("xhttp"), QStringLiteral("tls"),
                 QString(), [](XrayConfigModel &m) {
                     m.setXhttpMode(QStringLiteral("Packet-up"));
                     m.setXhttpSessionPlacement(QStringLiteral("Header"));
                     m.setXhttpSeqPlacement(QStringLiteral("Header"));
                     m.setXhttpUplinkDataPlacement(QStringLiteral("Header"));
                     m.setXPaddingObfsMode(true);
                     m.setXPaddingPlacement(QStringLiteral("Header"));
                 } };
    }

    inline ContractCtx kcpCtx(const QString &security)
    {
        return { QStringLiteral("mkcp/%1").arg(security), QStringLiteral("mkcp"), security, QString(), {} };
    }

    inline ContractCase kase(const ContractCtx &ctx, const QString &what, const ModelEdit &edit)
    {
        ContractCase c;
        c.name = ctx.label + QLatin1Char('/') + what;
        c.transport = ctx.transport;
        c.security = ctx.security;
        c.flow = ctx.flow;
        if (ctx.security == QLatin1String("none"))
            c.blocked().reject(QStringLiteral("vless without TLS"));
        const ModelEdit prep = ctx.prep;
        c.edit = [prep, edit](XrayConfigModel &m) {
            if (prep)
                prep(m);
            if (edit)
                edit(m);
        };
        return c;
    }

    inline ModelEdit set(Setter setter, const QString &value)
    {
        return [setter, value](XrayConfigModel &m) { (m.*setter)(value); };
    }

    inline QString shown(const QString &value)
    {
        if (value.isEmpty())
            return QStringLiteral("empty");
        if (value.size() > 24)
            return QStringLiteral("%1chars").arg(value.size());
        return value;
    }

    inline QStringList contractErrors(const ContractCase &c, amnezia::XrayServerConfig *outServer = nullptr)
    {
        amnezia::XrayProtocolConfig stored;
        stored.serverConfig = baseConfig(c.transport, c.security, c.flow);
        XrayConfigModel model;
        model.updateModel(amnezia::DockerContainer::Xray, stored);
        if (c.edit)
            c.edit(model);
        if (outServer)
            *outServer = model.getProtocolConfig().serverConfig;
        return model.validationErrors();
    }

    struct RangeExpect
    {
        AppVerdict app;
        CoreVerdict core;
        const char *message;
        const char *stricter;
        const char *known;
    };

    struct RangeInput
    {
        const char *min;
        const char *max;
        bool viaUi;
    };

    inline const RangeInput (&rangeInputs())[10]
    {
        static const RangeInput inputs[10] = {
            { "", "", true },        { "0", "0", true },    { "0", "100", true },
            { "100", "0", true },    { "1", "1", true },    { "400", "200", false },
            { "-1", "5", false },    { "abc", "5", false }, { "1", "2147483648", false },
            { "1", "9223372036854775808", false },
        };
        return inputs;
    }

    inline void addRange(QList<ContractCase> &out, const ContractCtx &ctx, const QString &field, const QString &ruleId,
                         Setter setMin, Setter setMax, const RangeExpect (&expect)[10])
    {
        for (int i = 0; i < 10; ++i) {
            const RangeInput &in = rangeInputs()[i];
            const QString min = QString::fromLatin1(in.min);
            const QString max = QString::fromLatin1(in.max);
            const QString what = min.isEmpty() && max.isEmpty() ? QStringLiteral("%1=empty").arg(field)
                                                                 : QStringLiteral("%1=%2..%3").arg(field, min, max);
            ContractCase c = kase(ctx, what, [setMin, setMax, min, max](XrayConfigModel &m) {
                (m.*setMin)(min);
                (m.*setMax)(max);
            });
            c.ruleId = ruleId;
            c.app = expect[i].app;
            c.core = expect[i].core;
            c.coreMessage = QString::fromLatin1(expect[i].message);
            c.stricterWhy = QString::fromLatin1(expect[i].stricter);
            c.knownIssue = QString::fromLatin1(expect[i].known);
            c.viaUi = in.viaUi;
            out.append(c);
        }
    }

    inline QList<ContractCase> buildContractCases()
    {
        using A = AppVerdict;
        using C = CoreVerdict;
        QList<ContractCase> out;

        const ContractCtx rawReality = rawCtx(QStringLiteral("reality"));
        const ContractCtx rawTls = rawCtx(QStringLiteral("tls"));
        const ContractCtx rawTlsNoFlow = rawCtx(QStringLiteral("tls"), QString());
        const ContractCtx rawNone = rawCtx(QStringLiteral("none"), QString());
        const ContractCtx xTlsOne = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Stream-one"));
        const ContractCtx xTlsPacket = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Packet-up"));
        const ContractCtx xTlsUp = xhttpCtx(QStringLiteral("tls"), QStringLiteral("Stream-up"));
        const ContractCtx xRealityOne = xhttpCtx(QStringLiteral("reality"), QStringLiteral("Stream-one"));
        const ContractCtx kcpTls = kcpCtx(QStringLiteral("tls"));

        for (const ContractCtx &ctx : { rawReality, rawTls, rawTlsNoFlow, rawNone, xTlsOne, xTlsPacket, xTlsUp,
                                        xRealityOne, kcpTls, xhttpHeaderKeysCtx() })
            out.append(kase(ctx, QStringLiteral("baseline"), {}));

        for (const QString port : { QStringLiteral("1"), QStringLiteral("65535") })
            out.append(kase(rawReality, QStringLiteral("port=") + port, set(&XrayConfigModel::setPort, port)));
        for (const QString port : { QStringLiteral("0"), QStringLiteral("65536"), QString() })
            out.append(kase(rawReality, QStringLiteral("port=") + shown(port), set(&XrayConfigModel::setPort, port))
                               .blocked().explore());
        for (const QString port : { QStringLiteral("-1"), QStringLiteral("abc") })
            out.append(kase(rawReality, QStringLiteral("port=") + port, set(&XrayConfigModel::setPort, port))
                               .blocked().explore().rawInput());

        const QString longName = QString(250, QLatin1Char('a')) + QStringLiteral(".com");
        struct SniRow
        {
            QString value;
            bool viaUi;
        };
        const QList<SniRow> badSni = {
            { QStringLiteral("*"), false },          { QStringLiteral("*.example.com"), false },
            { QStringLiteral("a b"), false },        { QStringLiteral("пример.рф"), false },
            { QStringLiteral("example.com."), true }, { QStringLiteral("12345"), true },
            { longName, true },
        };
        for (const ContractCtx &ctx : { rawTls, rawReality }) {
            const bool reality = ctx.security == QLatin1String("reality");
            for (const QString sni : { QStringLiteral("www.example.com"), QStringLiteral("xn--e1afmkfd.com"), QString() })
                out.append(kase(ctx, QStringLiteral("sni=") + shown(sni), set(&XrayConfigModel::setSni, sni)));
            out.append(kase(ctx, QStringLiteral("sni=xn--e1afmkfd.xn--p1ai"),
                            set(&XrayConfigModel::setSni, QStringLiteral("xn--e1afmkfd.xn--p1ai"))));
            for (const SniRow &row : badSni) {
                ContractCase c = kase(ctx, QStringLiteral("sni=") + shown(row.value), set(&XrayConfigModel::setSni, row.value))
                                         .blocked()
                                         .stricter(QStringLiteral("the core takes any serverName; the TLS handshake fails later"));
                c.viaUi = row.viaUi;
                out.append(c);
            }
            ContractCase ipv4 = kase(ctx, QStringLiteral("sni=203.0.113.10"),
                                     set(&XrayConfigModel::setSni, QStringLiteral("203.0.113.10")));
            ContractCase ipv6 = kase(ctx, QStringLiteral("sni=2001:db8::1"),
                                     set(&XrayConfigModel::setSni, QStringLiteral("2001:db8::1")));
            if (reality) {
                ipv4.rule(QStringLiteral("reality-sni-ip")).blocked()
                        .stricter(QStringLiteral("REALITY with an IP SNI builds but carries no traffic"));
                ipv6.rule(QStringLiteral("reality-server-required")).blocked().reject(QStringLiteral("valid value for \"target\""));
            }
            out.append(ipv4);
            out.append(ipv6);
        }

        const QStringList tls13Fingerprints = XrayConfigModel::realityFingerprintOptions();
        const QStringList tls12Fingerprints = { QStringLiteral("android"), QStringLiteral("360") };
        struct FpCtx
        {
            ContractCtx ctx;
            QString tls12Rule;
            QString tls12Why;
        };
        const QList<FpCtx> fpContexts = {
            { rawTls, QStringLiteral("vision-fingerprint-tls12"), QStringLiteral("vision needs TLS 1.3; the core fails at runtime") },
            { rawReality, QStringLiteral("reality-fingerprint-tls12"), QStringLiteral("REALITY needs TLS 1.3; the core fails at runtime") },
            { xTlsOne, QStringLiteral("xhttp-fingerprint-no-h2"), QStringLiteral("the fingerprint offers no h2 while ALPN is h2") },
        };
        for (const FpCtx &f : fpContexts) {
            for (const QString &fp : tls13Fingerprints)
                out.append(kase(f.ctx, QStringLiteral("fingerprint=") + fp, set(&XrayConfigModel::setFingerprint, fp)));
            for (const QString &fp : tls12Fingerprints)
                out.append(kase(f.ctx, QStringLiteral("fingerprint=") + fp, set(&XrayConfigModel::setFingerprint, fp))
                                   .rule(f.tls12Rule).blocked().stricter(f.tls12Why));
            ContractCase foo = kase(f.ctx, QStringLiteral("fingerprint=foo"),
                                    set(&XrayConfigModel::setFingerprint, QStringLiteral("foo")))
                                       .rule(QStringLiteral("tls-fingerprint-known")).blocked().rawInput();
            if (f.ctx.security == QLatin1String("reality"))
                foo.stricter(QStringLiteral("the generator sends chrome instead of an unknown REALITY fingerprint"));
            else
                foo.reject(QStringLiteral("unknown \"fingerprint\""));
            out.append(foo);
            out.append(kase(f.ctx, QStringLiteral("fingerprint=Chrome"), set(&XrayConfigModel::setFingerprint, QStringLiteral("Chrome")))
                               .rawInput());
        }
        out.append(kase(rawTls, QStringLiteral("fingerprint=unsafe"), set(&XrayConfigModel::setFingerprint, QStringLiteral("unsafe")))
                           .blocked().stricter(QStringLiteral("TLS accepts \"unsafe\", which turns uTLS off")).rawInput());
        out.append(kase(rawReality, QStringLiteral("fingerprint=unsafe"), set(&XrayConfigModel::setFingerprint, QStringLiteral("unsafe")))
                           .rule(QStringLiteral("reality-fingerprint-unsafe")).blocked()
                           .stricter(QStringLiteral("the generator sends chrome instead of \"unsafe\" for REALITY")).rawInput());
        for (const QString &fp : tls12Fingerprints) {
            out.append(kase(rawTlsNoFlow, QStringLiteral("fingerprint=") + fp, set(&XrayConfigModel::setFingerprint, fp)));
            out.append(kase(xTlsOne, QStringLiteral("alpn=http/1.1+fingerprint=") + fp, [fp](XrayConfigModel &m) {
                           m.setAlpn(QStringLiteral("http/1.1"));
                           m.setFingerprint(fp);
                       }).rule(QStringLiteral("xhttp-fingerprint-no-h2")));
            out.append(kase(kcpTls, QStringLiteral("fingerprint=") + fp, set(&XrayConfigModel::setFingerprint, fp))
                               .rule(QStringLiteral("tls-fingerprint-kcp-ignored")));
        }
        out.append(kase(kcpTls, QStringLiteral("fingerprint=foo"), set(&XrayConfigModel::setFingerprint, QStringLiteral("foo")))
                           .rule(QStringLiteral("tls-fingerprint-kcp-ignored")).blocked()
                           .stricter(QStringLiteral("mKCP does not write the fingerprint")).rawInput());

        for (const ContractCtx &ctx : { rawTls, xTlsOne }) {
            for (const QString alpn : { QStringLiteral("h2"), QStringLiteral("http/1.1"), QStringLiteral("h2,http/1.1"),
                                        QStringLiteral("HTTP/2"), QString() }) {
                ContractCase c = kase(ctx, QStringLiteral("alpn=") + shown(alpn), set(&XrayConfigModel::setAlpn, alpn));
                if (ctx.transport == QLatin1String("xhttp"))
                    c.rule(QStringLiteral("xhttp-http-version"));
                out.append(c);
            }
            out.append(kase(ctx, QStringLiteral("alpn=foo"), set(&XrayConfigModel::setAlpn, QStringLiteral("foo")))
                               .blocked().stricter(QStringLiteral("the core takes any ALPN token; the handshake fails later"))
                               .rawInput());
        }
        out.append(kase(rawReality, QStringLiteral("alpn=foo"), set(&XrayConfigModel::setAlpn, QStringLiteral("foo"))).rawInput());

        for (const QString host : { QString(), QStringLiteral("cdn.example.com"), QStringLiteral("203.0.113.10") })
            out.append(kase(xTlsOne, QStringLiteral("host=") + shown(host), set(&XrayConfigModel::setXhttpHost, host)));
        for (const QString host : { QStringLiteral("bad host"), QStringLiteral("a:80"),
                                    QStringLiteral("пример.рф") })
            out.append(kase(xTlsOne, QStringLiteral("host=") + host, set(&XrayConfigModel::setXhttpHost, host))
                               .blocked().stricter(QStringLiteral("the core takes any host string")).rawInput());
        out.append(kase(xTlsOne, QStringLiteral("host=::1"), set(&XrayConfigModel::setXhttpHost, QStringLiteral("::1")))
                           .blocked().stricter(QStringLiteral("an IPv6 host without brackets breaks the request URL")));
        for (const QString path : { QStringLiteral("/"), QString(), QStringLiteral("text"), QStringLiteral("/?ed=1"),
                                    QStringLiteral("/%"), QStringLiteral("/") + QString(2000, QLatin1Char('p')) })
            out.append(kase(xTlsOne, QStringLiteral("path=") + shown(path), set(&XrayConfigModel::setXhttpPath, path)));
        out.append(kase(xTlsOne, QStringLiteral("path=/a b"), set(&XrayConfigModel::setXhttpPath, QStringLiteral("/a b"))).rawInput());

        for (const QString security : { QStringLiteral("none"), QStringLiteral("tls"), QStringLiteral("reality") }) {
            for (const QString &mode : XrayConfigModel::xhttpModeOptions()) {
                QString ruleId = QStringLiteral("xhttp-mode-enum");
                if (mode == QLatin1String("Auto"))
                    ruleId = security == QLatin1String("reality") ? QStringLiteral("xhttp-auto-resolve-reality")
                                                                  : QStringLiteral("xhttp-auto-resolve-other");
                out.append(kase(xhttpCtx(security, mode), QStringLiteral("mode"), {}).rule(ruleId));
            }
        }
        out.append(kase(xTlsOne, QStringLiteral("mode=foo"), set(&XrayConfigModel::setXhttpMode, QStringLiteral("foo")))
                           .rule(QStringLiteral("xhttp-mode-enum")).blocked().reject(QStringLiteral("unsupported mode")).rawInput());
        out.append(kase(xTlsOne, QStringLiteral("mode=stream-one"), set(&XrayConfigModel::setXhttpMode, QStringLiteral("stream-one")))
                           .rawInput());
        out.append(kase(xTlsOne, QStringLiteral("mode=empty"), set(&XrayConfigModel::setXhttpMode, QString())).rawInput());

        for (const QString &mode : XrayConfigModel::xhttpModeOptions()) {
            const ContractCtx ctx = xhttpCtx(QStringLiteral("tls"), mode);
            for (const QString &method : XrayConfigModel::xhttpUplinkMethodOptions())
                out.append(kase(ctx, QStringLiteral("method=") + method, set(&XrayConfigModel::setXhttpUplinkMethod, method)));
            ContractCase get = kase(ctx, QStringLiteral("method=GET"), set(&XrayConfigModel::setXhttpUplinkMethod, QStringLiteral("GET")))
                                       .rule(QStringLiteral("xhttp-get-packet-up"));
            if (mode != QLatin1String("Packet-up"))
                get.blocked().stricter(QStringLiteral("the generator sends POST instead of GET outside packet-up"));
            out.append(get);
        }
        out.append(kase(xTlsPacket, QStringLiteral("method=get"), set(&XrayConfigModel::setXhttpUplinkMethod, QStringLiteral("get")))
                           .rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("method=foo"), set(&XrayConfigModel::setXhttpUplinkMethod, QStringLiteral("foo")))
                           .blocked().stricter(QStringLiteral("the core takes any HTTP method token")).rawInput());

        for (const QString &mode : XrayConfigModel::xhttpModeOptions()) {
            const ContractCtx ctx = xhttpCtx(QStringLiteral("tls"), mode);
            for (const QString &pl : XrayConfigModel::xhttpSessionPlacementOptions()) {
                out.append(kase(ctx, QStringLiteral("sessionPlacement=") + pl, set(&XrayConfigModel::setXhttpSessionPlacement, pl))
                                   .rule(QStringLiteral("xhttp-placement-enum")));
                out.append(kase(ctx, QStringLiteral("seqPlacement=") + pl, set(&XrayConfigModel::setXhttpSeqPlacement, pl))
                                   .rule(QStringLiteral("xhttp-placement-enum")));
            }
            for (const QString &pl : XrayConfigModel::xhttpUplinkDataPlacementOptions()) {
                ContractCase c = kase(ctx, QStringLiteral("uplinkDataPlacement=") + pl,
                                      set(&XrayConfigModel::setXhttpUplinkDataPlacement, pl));
                if ((pl == QLatin1String("Header") || pl == QLatin1String("Cookie")) && mode != QLatin1String("Packet-up"))
                    c.rule(QStringLiteral("xhttp-uplink-data-placement-packet-up"));
                out.append(c);
            }
        }
        out.append(kase(xTlsPacket, QStringLiteral("sessionPlacement=foo"),
                        set(&XrayConfigModel::setXhttpSessionPlacement, QStringLiteral("foo")))
                           .rule(QStringLiteral("xhttp-placement-enum")).blocked()
                           .reject(QStringLiteral("unsupported session placement")).rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("seqPlacement=foo"), set(&XrayConfigModel::setXhttpSeqPlacement, QStringLiteral("foo")))
                           .rule(QStringLiteral("xhttp-placement-enum")).blocked()
                           .reject(QStringLiteral("unsupported seq placement")).rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("sessionPlacement=None"),
                        set(&XrayConfigModel::setXhttpSessionPlacement, QStringLiteral("None")))
                           .rule(QStringLiteral("xhttp-placement-none-is-path")).rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("uplinkDataPlacement=foo"),
                        set(&XrayConfigModel::setXhttpUplinkDataPlacement, QStringLiteral("foo")))
                           .blocked().reject(QStringLiteral("unsupported uplink data placement")).rawInput());

        const ContractCtx keysCtx = xhttpHeaderKeysCtx();
        const QList<QPair<QString, Setter>> keyFields = {
            { QStringLiteral("sessionKey"), &XrayConfigModel::setXhttpSessionKey },
            { QStringLiteral("seqKey"), &XrayConfigModel::setXhttpSeqKey },
            { QStringLiteral("uplinkDataKey"), &XrayConfigModel::setXhttpUplinkDataKey },
            { QStringLiteral("xPaddingKey"), &XrayConfigModel::setXPaddingKey },
            { QStringLiteral("xPaddingHeader"), &XrayConfigModel::setXPaddingHeader },
        };
        for (const auto &field : keyFields) {
            for (const QString key : { QString(), QStringLiteral("x_ok"), QStringLiteral("X-Header"), QString(300, QLatin1Char('k')) })
                out.append(kase(keysCtx, field.first + QLatin1Char('=') + shown(key), set(field.second, key)));
            for (const QString key : { QStringLiteral("a b"), QStringLiteral("ключ"), QStringLiteral("a:b"),
                                       QStringLiteral("a;b") })
                out.append(kase(keysCtx, field.first + QLatin1Char('=') + key, set(field.second, key))
                                   .blocked().stricter(QStringLiteral("the core takes the name, but the request carries no traffic"))
                                   .rawInput());
        }

        const char *noRange = "Invalid integer range";
        const char *disabled = "cannot be disabled";
        const RangeExpect xPaddingBytes[10] = {
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, disabled, "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, disabled, "", "" },
            { A::Blocked, C::Reject, disabled, "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
        };
        addRange(out, xTlsOne, QStringLiteral("xPaddingBytes"), QStringLiteral("xhttp-padding-bytes"),
                 &XrayConfigModel::setXPaddingBytesMin, &XrayConfigModel::setXPaddingBytesMax, xPaddingBytes);

        const char *zeroPost = "0 in the range breaks the packet-up uplink at runtime";
        const char *smallPost = "below 1000 bytes per post the packet-up uplink stalls";
        const char *negativePost = "the client panics: scMaxEachPostBytes should be bigger than 0";
        const RangeExpect scMaxEachPostBytes[10] = {
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Accept, "", zeroPost, "" },
            { A::Blocked, C::Accept, "", zeroPost, "" },
            { A::Blocked, C::Accept, "", smallPost, "" },
            { A::Blocked, C::Accept, "", smallPost, "" },
            { A::Blocked, C::Accept, "", negativePost, "" },
            { A::Blocked, C::Reject, noRange, "", "" },
            { A::Blocked, C::Accept, "", smallPost, "" },
            { A::Blocked, C::Reject, noRange, "", "" },
        };
        addRange(out, xTlsPacket, QStringLiteral("scMaxEachPostBytes"), QStringLiteral("xhttp-sc-max-each-post-bytes"),
                 &XrayConfigModel::setXhttpScMaxEachPostBytesMin, &XrayConfigModel::setXhttpScMaxEachPostBytesMax,
                 scMaxEachPostBytes);

        const char *unchecked = "validationErrors does not check this range and the generator writes it as is";
        const RangeExpect unvalidated[10] = {
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, noRange, "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, noRange, "", unchecked },
        };
        addRange(out, xTlsPacket, QStringLiteral("scMinPostsIntervalMs"), QStringLiteral("xhttp-min-posts-interval-zero"),
                 &XrayConfigModel::setXhttpScMinPostsIntervalMsMin, &XrayConfigModel::setXhttpScMinPostsIntervalMsMax,
                 unvalidated);

        const char *zeroSecs = "0 in the range is only a core warning";
        const RangeExpect scStreamUpServerSecs[10] = {
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Accept, "", zeroSecs, "" },
            { A::Blocked, C::Accept, "", zeroSecs, "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, noRange, "", "" },
            { A::Ok, C::Accept, "", "", "" },
            { A::Blocked, C::Reject, noRange, "", "" },
        };
        addRange(out, xTlsUp, QStringLiteral("scStreamUpServerSecs"), QStringLiteral("xhttp-stream-up-server-secs-zero"),
                 &XrayConfigModel::setXhttpScStreamUpServerSecsMin, &XrayConfigModel::setXhttpScStreamUpServerSecsMax,
                 scStreamUpServerSecs);

        const QList<std::tuple<QString, Setter, Setter>> xmuxRanges = {
            { QStringLiteral("xmux.maxConcurrency"), &XrayConfigModel::setXmuxMaxConcurrencyMin, &XrayConfigModel::setXmuxMaxConcurrencyMax },
            { QStringLiteral("xmux.maxConnections"), &XrayConfigModel::setXmuxMaxConnectionsMin, &XrayConfigModel::setXmuxMaxConnectionsMax },
            { QStringLiteral("xmux.cMaxReuseTimes"), &XrayConfigModel::setXmuxCMaxReuseTimesMin, &XrayConfigModel::setXmuxCMaxReuseTimesMax },
            { QStringLiteral("xmux.hMaxRequestTimes"), &XrayConfigModel::setXmuxHMaxRequestTimesMin, &XrayConfigModel::setXmuxHMaxRequestTimesMax },
            { QStringLiteral("xmux.hMaxReusableSecs"), &XrayConfigModel::setXmuxHMaxReusableSecsMin, &XrayConfigModel::setXmuxHMaxReusableSecsMax },
        };
        for (const auto &r : xmuxRanges)
            addRange(out, xTlsOne, std::get<0>(r), QStringLiteral("xmux-zero-unlimited"), std::get<1>(r), std::get<2>(r), unvalidated);

        out.append(kase(xTlsOne, QStringLiteral("xmux=concurrency+connections"), [](XrayConfigModel &m) {
                       m.setXmuxEnabled(true);
                       m.setXmuxMaxConcurrencyMin(QStringLiteral("1"));
                       m.setXmuxMaxConcurrencyMax(QStringLiteral("1"));
                       m.setXmuxMaxConnectionsMin(QStringLiteral("1"));
                       m.setXmuxMaxConnectionsMax(QStringLiteral("1"));
                   }).rule(QStringLiteral("xmux-exclusive")).blocked().reject(QStringLiteral("maxConnections cannot be specified together")));
        out.append(kase(xTlsOne, QStringLiteral("xmux=off+concurrency+connections"), [](XrayConfigModel &m) {
                       m.setXmuxMaxConcurrencyMin(QStringLiteral("1"));
                       m.setXmuxMaxConcurrencyMax(QStringLiteral("1"));
                       m.setXmuxMaxConnectionsMin(QStringLiteral("1"));
                       m.setXmuxMaxConnectionsMax(QStringLiteral("1"));
                       m.setXmuxEnabled(false);
                   }).rule(QStringLiteral("xmux-empty-defaults")));

        for (const QString v : { QString(), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("65536") })
            out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=") + shown(v), set(&XrayConfigModel::setXhttpUplinkChunkSize, v)));
        for (const QString v : { QStringLiteral("-1"), QStringLiteral("2147483648") })
            out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=") + v, set(&XrayConfigModel::setXhttpUplinkChunkSize, v))
                               .rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("uplinkChunkSize=abc"),
                        set(&XrayConfigModel::setXhttpUplinkChunkSize, QStringLiteral("abc")))
                           .blocked().stricter(QStringLiteral("the generator writes 0 for a non-number")).rawInput());

        for (const QString v : { QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("100000") })
            out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=") + v, set(&XrayConfigModel::setXhttpScMaxBufferedPosts, v))
                               .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")));
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=100001"),
                        set(&XrayConfigModel::setXhttpScMaxBufferedPosts, QStringLiteral("100001")))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")).blocked()
                           .stricter(QStringLiteral("the server preallocates a buffer per post")));
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=-1"),
                        set(&XrayConfigModel::setXhttpScMaxBufferedPosts, QStringLiteral("-1")))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")).blocked()
                           .stricter(QStringLiteral("a negative value carries no traffic")).rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=abc"),
                        set(&XrayConfigModel::setXhttpScMaxBufferedPosts, QStringLiteral("abc")))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")).blocked()
                           .stricter(QStringLiteral("the generator drops a non-number")).rawInput());
        out.append(kase(xTlsPacket, QStringLiteral("scMaxBufferedPosts=9223372036854775808"),
                        set(&XrayConfigModel::setXhttpScMaxBufferedPosts, QStringLiteral("9223372036854775808")))
                           .rule(QStringLiteral("xhttp-max-buffered-posts-prealloc")).rawInput());

        for (const QString v : { QStringLiteral("0"), QStringLiteral("30"), QStringLiteral("-1") })
            out.append(kase(xTlsOne, QStringLiteral("hKeepAlivePeriod=") + v, set(&XrayConfigModel::setXmuxHKeepAlivePeriod, v)));
        out.append(kase(xTlsOne, QStringLiteral("hKeepAlivePeriod=abc"),
                        set(&XrayConfigModel::setXmuxHKeepAlivePeriod, QStringLiteral("abc")))
                           .blocked().stricter(QStringLiteral("the generator writes 0 for a non-number")).rawInput());
        out.append(kase(xTlsOne, QStringLiteral("hKeepAlivePeriod=9223372036854775808"),
                        set(&XrayConfigModel::setXmuxHKeepAlivePeriod, QStringLiteral("9223372036854775808")))
                           .rawInput());
        out.append(kase(xTlsOne, QStringLiteral("alpn=http/1.1+hKeepAlivePeriod=30"), [](XrayConfigModel &m) {
                       m.setAlpn(QStringLiteral("http/1.1"));
                       m.setXmuxHKeepAlivePeriod(QStringLiteral("30"));
                   }).rule(QStringLiteral("use-keepalive-h1-alpn")));
        out.append(kase(xhttpCtx(QStringLiteral("none"), QStringLiteral("Stream-one")), QStringLiteral("hKeepAlivePeriod=30"),
                        set(&XrayConfigModel::setXmuxHKeepAlivePeriod, QStringLiteral("30")))
                           .rule(QStringLiteral("use-keepalive-h1-none")));

        struct KcpRow
        {
            QString field;
            Setter setter;
            QString ruleId;
            QStringList ok;
            QStringList clamped;
            QStringList rawClamped;
            QString why;
        };
        const QList<KcpRow> kcpRows = {
            { QStringLiteral("mtu"), &XrayConfigModel::setMkcpMtu, QStringLiteral("kcp-mtu"),
              { QString(), QStringLiteral("576"), QStringLiteral("1460") },
              { QStringLiteral("20"), QStringLiteral("21"), QStringLiteral("575"), QStringLiteral("1461") },
              { QStringLiteral("abc"), QStringLiteral("-1") },
              QStringLiteral("UI range 576-1460 is narrower than the core (>= 21); the generator clamps") },
            { QStringLiteral("tti"), &XrayConfigModel::setMkcpTti, QStringLiteral("kcp-tti"),
              { QStringLiteral("10"), QStringLiteral("100") },
              { QStringLiteral("9"), QStringLiteral("101"), QStringLiteral("1000"), QStringLiteral("1001") },
              {},
              QStringLiteral("UI range 10-100 is narrower than the core (10-1000); the generator clamps") },
            { QStringLiteral("cwndMultiplier"), &XrayConfigModel::setMkcpCwndMultiplier, QStringLiteral("kcp-cwnd"),
              { QStringLiteral("1"), QStringLiteral("5000") },
              { QStringLiteral("0") },
              { QStringLiteral("-1") },
              QStringLiteral("the generator raises cwndMultiplier to 1") },
            { QStringLiteral("maxSendingWindow"), &XrayConfigModel::setMkcpMaxSendingWindow, QStringLiteral("kcp-sending-window"),
              { QString(), QStringLiteral("1350"), QStringLiteral("2147483647") },
              { QStringLiteral("1349") },
              { QStringLiteral("abc"), QStringLiteral("-1") },
              QStringLiteral("the generator omits a window below MTU") },
        };
        for (const KcpRow &row : kcpRows) {
            for (const QString &v : row.ok)
                out.append(kase(kcpTls, row.field + QLatin1Char('=') + shown(v), set(row.setter, v)).rule(row.ruleId));
            for (const QString &v : row.clamped)
                out.append(kase(kcpTls, row.field + QLatin1Char('=') + v, set(row.setter, v)).rule(row.ruleId).blocked()
                                   .stricter(row.why));
            for (const QString &v : row.rawClamped)
                out.append(kase(kcpTls, row.field + QLatin1Char('=') + v, set(row.setter, v)).rule(row.ruleId).blocked()
                                   .stricter(row.why).rawInput());
        }
        out.append(kase(kcpTls, QStringLiteral("cwndMultiplier=5001"), set(&XrayConfigModel::setMkcpCwndMultiplier, QStringLiteral("5001")))
                           .rule(QStringLiteral("kcp-cwnd-overflow")).blocked()
                           .stricter(QStringLiteral("large cwndMultiplier overflows the window at runtime")));
        for (const QPair<QString, Setter> &cap : QList<QPair<QString, Setter>> {
                     { QStringLiteral("uplinkCapacity"), &XrayConfigModel::setMkcpUplinkCapacity },
                     { QStringLiteral("downlinkCapacity"), &XrayConfigModel::setMkcpDownlinkCapacity } }) {
            for (const QString v : { QStringLiteral("0"), QStringLiteral("4095") })
                out.append(kase(kcpTls, cap.first + QLatin1Char('=') + v, set(cap.second, v)).rule(QStringLiteral("kcp-capacity-overflow")));
            out.append(kase(kcpTls, cap.first + QStringLiteral("=4096"), set(cap.second, QStringLiteral("4096")))
                               .rule(QStringLiteral("kcp-capacity-overflow")).blocked()
                               .stricter(QStringLiteral("capacity above 4095 overflows at runtime")));
            out.append(kase(kcpTls, cap.first + QStringLiteral("=-1"), set(cap.second, QStringLiteral("-1")))
                               .rule(QStringLiteral("kcp-capacity-overflow")).blocked().reject().rawInput());
            out.append(kase(kcpTls, cap.first + QStringLiteral("=abc"), set(cap.second, QStringLiteral("abc")))
                               .rule(QStringLiteral("kcp-capacity-overflow")).blocked()
                               .stricter(QStringLiteral("the generator writes 0 for a non-number")).rawInput());
        }

        for (const QString &pl : XrayConfigModel::xPaddingPlacementOptions()) {
            for (const QString &method : XrayConfigModel::xPaddingMethodOptions())
                out.append(kase(xTlsOne, QStringLiteral("xPadding=%1/%2").arg(pl, method), [pl, method](XrayConfigModel &m) {
                               m.setXPaddingObfsMode(true);
                               m.setXPaddingPlacement(pl);
                               m.setXPaddingMethod(method);
                           }).rule(QStringLiteral("xhttp-padding-placement")));
        }
        out.append(kase(xTlsOne, QStringLiteral("xPaddingPlacement=foo"), [](XrayConfigModel &m) {
                       m.setXPaddingObfsMode(true);
                       m.setXPaddingPlacement(QStringLiteral("foo"));
                   }).rule(QStringLiteral("xhttp-padding-placement")).blocked().reject(QStringLiteral("unsupported padding placement"))
                           .rawInput());
        out.append(kase(xTlsOne, QStringLiteral("xPaddingMethod=foo"), [](XrayConfigModel &m) {
                       m.setXPaddingObfsMode(true);
                       m.setXPaddingMethod(QStringLiteral("foo"));
                   }).rule(QStringLiteral("xhttp-padding-method")).blocked().reject(QStringLiteral("unsupported padding method"))
                           .rawInput());
        out.append(kase(xTlsOne, QStringLiteral("obfs off+xPaddingPlacement=foo"), [](XrayConfigModel &m) {
                       m.setXPaddingObfsMode(false);
                       m.setXPaddingPlacement(QStringLiteral("foo"));
                   }).rule(QStringLiteral("use-padding-obfs-fields")).rawInput());
        out.append(kase(xTlsOne, QStringLiteral("obfs off+xPaddingBytes=200..400"), [](XrayConfigModel &m) {
                       m.setXPaddingObfsMode(false);
                       m.setXPaddingBytesMin(QStringLiteral("200"));
                       m.setXPaddingBytesMax(QStringLiteral("400"));
                   }).rule(QStringLiteral("use-padding-bytes-without-obfs")));
        out.append(kase(xTlsOne, QStringLiteral("xPaddingMethod=Random"), set(&XrayConfigModel::setXPaddingMethod, QStringLiteral("Random")))
                           .rawInput());

        const QString vision = QStringLiteral("xtls-rprx-vision");
        for (const ContractCtx &ctx : { rawTlsNoFlow, rawCtx(QStringLiteral("reality"), QString()) }) {
            for (const QString &flow : XrayConfigModel::flowOptions()) {
                ContractCase c = kase(ctx, QStringLiteral("flow=") + shown(flow), set(&XrayConfigModel::setFlow, flow));
                if (flow.endsWith(QLatin1String("-udp443")))
                    c.rule(ctx.security == QLatin1String("tls") ? QStringLiteral("vision-udp443")
                                                                : QStringLiteral("vision-udp-via-xudp"));
                out.append(c);
            }
            out.append(kase(ctx, QStringLiteral("flow=foo"), set(&XrayConfigModel::setFlow, QStringLiteral("foo")))
                               .blocked().reject(QStringLiteral("doesn't support")).rawInput());
        }
        out.append(kase(rawNone, QStringLiteral("flow=vision"), set(&XrayConfigModel::setFlow, vision))
                           .rule(QStringLiteral("vision-needs-security")));
        out.append(kase(xTlsOne, QStringLiteral("flow=vision"), set(&XrayConfigModel::setFlow, vision))
                           .rule(QStringLiteral("vision-needs-raw")));
        out.append(kase(kcpTls, QStringLiteral("flow=vision"), set(&XrayConfigModel::setFlow, vision))
                           .rule(QStringLiteral("vision-needs-raw")));
        out.append(kase(rawNone, QStringLiteral("flow=foo"), set(&XrayConfigModel::setFlow, QStringLiteral("foo")))
                           .blocked().stricter(QStringLiteral("the generator drops flow without TLS or REALITY")).rawInput());
        out.append(kase(xTlsOne, QStringLiteral("flow=foo"), set(&XrayConfigModel::setFlow, QStringLiteral("foo")))
                           .blocked().stricter(QStringLiteral("the generator drops flow outside raw")).rawInput());

        out.append(kase(rawTls, QStringLiteral("security=foo"), set(&XrayConfigModel::setSecurity, QStringLiteral("foo")))
                           .blocked().reject(QStringLiteral("Unknown security")).rawInput());
        out.append(kase(rawReality, QStringLiteral("transport=foo"), set(&XrayConfigModel::setTransport, QStringLiteral("foo")))
                           .blocked().stricter(QStringLiteral("the generator falls back to raw")).rawInput());
        out.append(kase(kcpTls, QStringLiteral("security=reality"), set(&XrayConfigModel::setSecurity, QStringLiteral("reality")))
                           .rule(QStringLiteral("reality-network")).blocked().reject(QStringLiteral("vless without TLS")));

        return out;
    }

    inline const QList<ContractCase> &contractCases()
    {
        static const QList<ContractCase> all = buildContractCases();
        return all;
    }
}

#endif

#include "xrayInboundValidator.h"

#include <QCoreApplication>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>

#include "logger.h"

#include "core/models/protocols/xrayInbound.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"

namespace
{
    Logger logger("XrayInboundValidator");

    QString tr(const char *text)
    {
        return QCoreApplication::translate("XrayInboundValidator", text);
    }

    bool isValidDomainName(const QString &text)
    {
        static const QRegularExpression label(QStringLiteral(R"(^[A-Za-z0-9_]([A-Za-z0-9_-]{0,61}[A-Za-z0-9])?$)"));
        static const QRegularExpression topLevel(QStringLiteral(R"(^([A-Za-z]{2,63}|xn--[A-Za-z0-9-]{1,59})$)"));
        const QStringList labels = text.split(QLatin1Char('.'));
        if (labels.size() < 2) {
            return false;
        }
        for (int i = 0; i < labels.size() - 1; ++i) {
            if (!label.match(labels.at(i)).hasMatch()) {
                return false;
            }
        }
        return topLevel.match(labels.last()).hasMatch();
    }

    bool isValidHost(const QString &text, bool allowIpv6 = false)
    {
        if (text.isEmpty() || text.length() > 253 || text.contains(QLatin1Char('*'))) {
            return false;
        }
        const QHostAddress address(text);
        if (address.protocol() == QHostAddress::IPv4Protocol) {
            return NetworkUtilities::checkIPv4Format(text);
        }
        if (address.protocol() == QHostAddress::IPv6Protocol) {
            return allowIpv6;
        }
        static const QRegularExpression onlyDigits(QStringLiteral(R"(^\d+$)"));
        return !onlyDigits.match(text).hasMatch() && isValidDomainName(text);
    }

    bool isIpAddress(const QString &text)
    {
        const QHostAddress address(text);
        return address.protocol() == QHostAddress::IPv4Protocol || address.protocol() == QHostAddress::IPv6Protocol;
    }

    struct IntRange
    {
        bool present = false;
        bool valid = true;
        qint64 from = 0;
        qint64 to = 0;
    };

    IntRange readRange(const QJsonValue &value, bool allowNegative)
    {
        IntRange range;
        if (value.isUndefined() || value.isNull()) {
            return range;
        }
        range.present = true;
        constexpr double int32Max = 2147483647;
        if (value.isDouble()) {
            const double number = value.toDouble();
            range.valid = std::abs(number) <= int32Max && number == static_cast<qint64>(number)
                    && (allowNegative || number >= 0);
            range.from = range.to = range.valid ? static_cast<qint64>(number) : 0;
            return range;
        }
        static const QRegularExpression unsignedRange(QStringLiteral(R"(^(\d+)(?:-(\d+))?$)"));
        static const QRegularExpression signedRange(QStringLiteral(R"(^(-?\d+)(?:-(-?\d+))?$)"));
        const QRegularExpressionMatch match =
                (allowNegative ? signedRange : unsignedRange).match(value.toString().trimmed());
        if (!value.isString() || !match.hasMatch()) {
            range.valid = false;
            return range;
        }
        bool fromOk = false;
        bool toOk = true;
        range.from = match.captured(1).toLongLong(&fromOk);
        range.to = match.captured(2).isEmpty() ? range.from : match.captured(2).toLongLong(&toOk);
        range.valid = fromOk && toOk && std::abs(range.from) <= int32Max && std::abs(range.to) <= int32Max;
        return range;
    }

    class LineTrackingParser
    {
    public:
        explicit LineTrackingParser(const QString &text) : m_text(text)
        {
        }

        QJsonValue parse()
        {
            const QJsonValue value = parseValue(QString());
            if (m_failed) {
                return {};
            }
            skipSpaces();
            if (m_pos < m_text.size()) {
                fail(tr("unexpected text after the end of JSON"));
            }
            return value;
        }

        bool failed() const
        {
            return m_failed;
        }
        int errorLine() const
        {
            return m_errorLine;
        }
        int errorColumn() const
        {
            return m_errorColumn;
        }
        QString errorMessage() const
        {
            return m_errorMessage;
        }
        const QHash<QString, int> &lines() const
        {
            return m_lines;
        }

    private:
        QChar peek() const
        {
            return m_pos < m_text.size() ? m_text.at(m_pos) : QChar();
        }

        QChar advance()
        {
            const QChar c = m_text.at(m_pos++);
            if (c == QLatin1Char('\n')) {
                ++m_line;
                m_column = 1;
            } else {
                ++m_column;
            }
            return c;
        }

        void skipSpaces()
        {
            while (m_pos < m_text.size() && peek().isSpace()) {
                advance();
            }
        }

        void fail(const QString &message, int line = 0, int column = 0)
        {
            if (m_failed) {
                return;
            }
            m_failed = true;
            m_errorLine = line > 0 ? line : m_line;
            m_errorColumn = column > 0 ? column : m_column;
            m_errorMessage = message;
        }

        void failMissingComma()
        {
            fail(tr("missing comma after %1").arg(m_lastToken), m_lastTokenLine, m_lastTokenColumn);
        }

        void remember(const QString &token, int line, int column)
        {
            m_lastToken = token.size() > 40 ? token.left(37) + QStringLiteral("...") : token;
            m_lastTokenLine = line;
            m_lastTokenColumn = column;
        }

        QString parseString()
        {
            if (peek() != QLatin1Char('"')) {
                fail(peek() == QLatin1Char('\'') ? tr("keys and strings use double quotes")
                                                 : tr("a string in double quotes was expected"));
                return {};
            }
            advance();
            QString out;
            while (true) {
                if (m_pos >= m_text.size()) {
                    fail(tr("missing closing quote"));
                    return {};
                }
                const QChar c = advance();
                if (c == QLatin1Char('"')) {
                    return out;
                }
                if (c == QLatin1Char('\n')) {
                    fail(tr("missing closing quote"), m_line - 1);
                    return {};
                }
                if (c != QLatin1Char('\\')) {
                    out.append(c);
                    continue;
                }
                if (m_pos >= m_text.size()) {
                    fail(tr("missing closing quote"));
                    return {};
                }
                const QChar e = advance();
                switch (e.unicode()) {
                case 'n': out.append(QLatin1Char('\n')); break;
                case 't': out.append(QLatin1Char('\t')); break;
                case 'r': out.append(QLatin1Char('\r')); break;
                case 'b': out.append(QLatin1Char('\b')); break;
                case 'f': out.append(QLatin1Char('\f')); break;
                case '"': out.append(QLatin1Char('"')); break;
                case '\\': out.append(QLatin1Char('\\')); break;
                case '/': out.append(QLatin1Char('/')); break;
                case 'u': {
                    const QString hex = m_text.mid(m_pos, 4);
                    bool ok = false;
                    const ushort code = hex.toUShort(&ok, 16);
                    if (!ok || hex.size() != 4) {
                        fail(tr("invalid escape sequence"));
                        return {};
                    }
                    for (int k = 0; k < 4; ++k) {
                        advance();
                    }
                    out.append(QChar(code));
                    break;
                }
                default: fail(tr("invalid escape sequence")); return {};
                }
            }
        }

        QJsonValue parseObject(const QString &path)
        {
            advance();
            QJsonObject object;
            skipSpaces();
            if (peek() == QLatin1Char('}')) {
                advance();
                return object;
            }
            while (true) {
                skipSpaces();
                if (peek() == QLatin1Char('}')) {
                    fail(tr("unexpected comma before }"), m_lastTokenLine, m_lastTokenColumn);
                    return {};
                }
                const int keyLine = m_line;
                const int keyColumn = m_column;
                const QString key = parseString();
                if (m_failed) {
                    return {};
                }
                const QString childPath = path.isEmpty() ? key : path + QLatin1Char('.') + key;
                if (object.contains(key)) {
                    fail(tr("duplicate key \"%1\"").arg(key), keyLine, keyColumn);
                    return {};
                }
                m_lines.insert(childPath, keyLine);
                remember(QStringLiteral("\"%1\"").arg(key), keyLine, keyColumn);
                skipSpaces();
                if (peek() != QLatin1Char(':')) {
                    fail(tr("missing colon after key \"%1\"").arg(key), keyLine, keyColumn);
                    return {};
                }
                advance();
                const QJsonValue value = parseValue(childPath);
                if (m_failed) {
                    return {};
                }
                object.insert(key, value);
                skipSpaces();
                if (peek() == QLatin1Char(',')) {
                    advance();
                    continue;
                }
                if (peek() == QLatin1Char('}')) {
                    advance();
                    return object;
                }
                if (m_pos >= m_text.size()) {
                    fail(tr("missing closing }"));
                } else {
                    failMissingComma();
                }
                return {};
            }
        }

        QJsonValue parseArray(const QString &path)
        {
            advance();
            QJsonArray array;
            skipSpaces();
            if (peek() == QLatin1Char(']')) {
                advance();
                return array;
            }
            while (true) {
                skipSpaces();
                if (peek() == QLatin1Char(']')) {
                    fail(tr("unexpected comma before ]"), m_lastTokenLine, m_lastTokenColumn);
                    return {};
                }
                const QJsonValue value = parseValue(QStringLiteral("%1[%2]").arg(path).arg(array.size()));
                if (m_failed) {
                    return {};
                }
                array.append(value);
                skipSpaces();
                if (peek() == QLatin1Char(',')) {
                    advance();
                    continue;
                }
                if (peek() == QLatin1Char(']')) {
                    advance();
                    return array;
                }
                if (m_pos >= m_text.size()) {
                    fail(tr("missing closing ]"));
                } else {
                    failMissingComma();
                }
                return {};
            }
        }

        QJsonValue parseValue(const QString &path)
        {
            skipSpaces();
            if (!path.isEmpty() && !m_lines.contains(path)) {
                m_lines.insert(path, m_line);
            }
            if (m_pos >= m_text.size()) {
                fail(tr("JSON ends unexpectedly"));
                return {};
            }
            const QChar c = peek();
            if (c == QLatin1Char('{')) {
                return parseObject(path);
            }
            if (c == QLatin1Char('[')) {
                return parseArray(path);
            }
            const int line = m_line;
            const int column = m_column;
            if (c == QLatin1Char('"')) {
                const QString value = parseString();
                remember(QStringLiteral("\"%1\"").arg(value), line, column);
                return value;
            }
            static const QRegularExpression number(QStringLiteral("^-?(0|[1-9]\\d*)(\\.\\d+)?([eE][+-]?\\d+)?"));
            const QRegularExpressionMatch match = number.match(m_text.mid(m_pos, 64));
            if (match.hasMatch() && match.capturedLength() > 0) {
                const QString token = match.captured(0);
                for (int k = 0; k < token.size(); ++k) {
                    advance();
                }
                remember(token, line, column);
                return token.toDouble();
            }
            for (const auto &[word, value] :
                 { std::pair<QLatin1String, QJsonValue> { QLatin1String("true"), true },
                   std::pair<QLatin1String, QJsonValue> { QLatin1String("false"), false },
                   std::pair<QLatin1String, QJsonValue> { QLatin1String("null"), QJsonValue() } }) {
                if (QStringView(m_text).mid(m_pos).startsWith(word)) {
                    for (int k = 0; k < word.size(); ++k) {
                        advance();
                    }
                    remember(word, line, column);
                    return value;
                }
            }
            if (c == QLatin1Char('\'')) {
                fail(tr("keys and strings use double quotes"));
            } else {
                fail(tr("unexpected value"));
            }
            return {};
        }

        const QString &m_text;
        int m_pos = 0;
        int m_line = 1;
        int m_column = 1;
        bool m_failed = false;
        int m_errorLine = 1;
        int m_errorColumn = 1;
        QString m_errorMessage;
        QString m_lastToken;
        int m_lastTokenLine = 1;
        int m_lastTokenColumn = 1;
        QHash<QString, int> m_lines;
    };

    class Rules
    {
    public:
        Rules(const QJsonObject &inbound, const QHash<QString, int> &lines, QList<amnezia::XrayInboundIssue> &issues)
            : m_inbound(inbound), m_lines(lines), m_issues(issues)
        {
        }

        void run()
        {
            using Level = amnezia::XrayInboundIssue::Level;

            if (m_inbound.value(QStringLiteral("protocol")).toString() != QLatin1String("vless")) {
                add(Level::Error, QStringLiteral("protocol"), tr("protocol must be \"vless\""));
            }

            const QJsonValue port = m_inbound.value(QStringLiteral("port"));
            const double portNumber = port.toDouble(-1);
            if (!port.isDouble() || portNumber != static_cast<int>(portNumber) || portNumber < 1 || portNumber > 65535) {
                add(Level::Error, QStringLiteral("port"), tr("port must be a number from 1 to 65535"));
            }

            const QJsonObject settings = m_inbound.value(QStringLiteral("settings")).toObject();
            const QString decryption = settings.value(QStringLiteral("decryption")).toString();
            if (decryption != QLatin1String("none") && !decryption.startsWith(QLatin1String("mlkem768x25519plus"))) {
                add(Level::Error, QStringLiteral("settings.decryption"), tr("settings.decryption must be \"none\""));
            }
            const QJsonArray clients = settings.value(QStringLiteral("clients")).toArray();
            for (int k = 0; k < clients.size(); ++k) {
                const QJsonObject client = clients.at(k).toObject();
                if (client.contains(QStringLiteral("encryption"))) {
                    add(Level::Error, QStringLiteral("settings.clients[%1]").arg(k),
                        tr("clients must not have an encryption field"));
                }
                if (client.value(QStringLiteral("id")).toString() != QLatin1String(amnezia::xrayInbound::autoValue)) {
                    add(Level::Error, QStringLiteral("settings.clients[%1].id").arg(k),
                        tr("clients id must be \"@auto\", the app fills in client ids"));
                }
            }
            const QString flow =
                    clients.isEmpty() ? QString() : clients.first().toObject().value(QStringLiteral("flow")).toString();
            const QString flowPath = QStringLiteral("settings.clients[0].flow");

            const QString sp = QStringLiteral("streamSettings");
            if (!m_inbound.value(sp).isObject()) {
                add(Level::Error, sp, tr("streamSettings is required"));
                return;
            }
            const QJsonObject stream = m_inbound.value(sp).toObject();
            const QString network = stream.value(QStringLiteral("network")).toString(QStringLiteral("raw")).toLower();
            const QString security = stream.value(QStringLiteral("security")).toString(QStringLiteral("none")).toLower();
            const QString networkPath = sp + QStringLiteral(".network");
            const QString securityPath = sp + QStringLiteral(".security");

            static const QStringList removedNetworks { QStringLiteral("http"), QStringLiteral("h2"),
                                                       QStringLiteral("h3"), QStringLiteral("quic") };
            static const QStringList supportedNetworks { QStringLiteral("raw"), QStringLiteral("tcp"),
                                                         QStringLiteral("xhttp"), QStringLiteral("kcp") };
            if (removedNetworks.contains(network)) {
                add(Level::Error, networkPath, tr("%1 transport was removed from XRay core, use xhttp").arg(network));
            } else if (!supportedNetworks.contains(network)) {
                add(Level::Error, networkPath, tr("network \"%1\" is not supported, use raw, xhttp or kcp").arg(network));
            }

            if (security == QLatin1String("xtls")) {
                add(Level::Error, securityPath, tr("security \"xtls\" was removed from XRay core, use tls or reality"));
            } else if (security != QLatin1String("none") && security != QLatin1String("tls")
                       && security != QLatin1String("reality")) {
                add(Level::Error, securityPath, tr("unknown security \"%1\"").arg(security));
            }
            const bool rawNetwork = network == QLatin1String("raw") || network == QLatin1String("tcp");
            if (security == QLatin1String("reality") && !rawNetwork && network != QLatin1String("xhttp")) {
                add(Level::Error, securityPath, tr("Reality works only with RAW (TCP) and XHTTP transport"));
            }
            if (security == QLatin1String("none")) {
                add(Level::Error, securityPath, tr("security none is not supported by XRay 26.10, use tls or reality"));
            }

            if (!flow.isEmpty()) {
                const QString visionMessage =
                        tr("xtls-rprx-vision works only with RAW (TCP) transport and TLS or Reality security");
                if (flow == QLatin1String("xtls-rprx-vision-udp443")) {
                    add(Level::Error, flowPath,
                        tr("xtls-rprx-vision-udp443 is a client flow, the server takes xtls-rprx-vision"));
                } else if (flow != QLatin1String("xtls-rprx-vision")) {
                    add(Level::Error, flowPath, tr("unknown flow \"%1\"").arg(flow));
                } else if (!rawNetwork) {
                    add(Level::Error, networkPath, visionMessage);
                } else if (security != QLatin1String("tls") && security != QLatin1String("reality")) {
                    add(Level::Error, securityPath, visionMessage);
                }
            }

            if (security == QLatin1String("reality")) {
                checkReality(stream, sp + QStringLiteral(".realitySettings"));
            }
            if (security == QLatin1String("tls")) {
                checkTls(stream.value(QStringLiteral("tlsSettings")).toObject(), sp + QStringLiteral(".tlsSettings"));
            }
            if (network == QLatin1String("xhttp")) {
                checkXhttp(stream.value(QStringLiteral("xhttpSettings")).toObject(),
                           sp + QStringLiteral(".xhttpSettings"), security);
            }
            if (network == QLatin1String("kcp")) {
                checkKcp(stream.value(QStringLiteral("kcpSettings")).toObject(), sp + QStringLiteral(".kcpSettings"));
            }
        }

    private:
        void checkReality(const QJsonObject &stream, const QString &rp)
        {
            using Level = amnezia::XrayInboundIssue::Level;
            if (!stream.value(QStringLiteral("realitySettings")).isObject()) {
                add(Level::Error, QStringLiteral("streamSettings.security"),
                    tr("realitySettings is required for Reality"));
                return;
            }
            const QJsonObject reality = stream.value(QStringLiteral("realitySettings")).toObject();
            if (reality.value(QStringLiteral("target")).toString().isEmpty()
                && reality.value(QStringLiteral("dest")).toString().isEmpty()) {
                add(Level::Error, rp, tr("realitySettings.target is required"));
            }
            const QJsonArray serverNames = reality.value(QStringLiteral("serverNames")).toArray();
            if (serverNames.isEmpty()) {
                add(Level::Error, rp, tr("serverNames must not be empty"));
            }
            for (int k = 0; k < serverNames.size(); ++k) {
                const QString name = serverNames.at(k).toString().trimmed();
                const QString namePath = rp + QStringLiteral(".serverNames[%1]").arg(k);
                if (isIpAddress(name)) {
                    add(Level::Error, namePath, tr("Reality needs a domain name in serverNames, not an IP address"));
                } else if (!isValidHost(name)) {
                    add(Level::Error, namePath, tr("serverNames: enter a valid domain name"));
                }
            }
            if (reality.value(QStringLiteral("privateKey")).toString().isEmpty()) {
                add(Level::Error, rp, tr("privateKey is required, keep @auto to use the server key"));
            }
            const QJsonArray shortIds = reality.value(QStringLiteral("shortIds")).toArray();
            if (shortIds.isEmpty()) {
                add(Level::Error, rp, tr("shortIds must not be empty"));
            }
            static const QRegularExpression hex(QStringLiteral("^[0-9a-fA-F]{0,16}$"));
            for (int k = 0; k < shortIds.size(); ++k) {
                const QString id = shortIds.at(k).toString();
                if (id != QLatin1String("@auto") && !hex.match(id).hasMatch()) {
                    add(Level::Error, rp + QStringLiteral(".shortIds[%1]").arg(k),
                        tr("shortId must be hex, up to 16 characters"));
                }
            }
        }

        void checkTls(const QJsonObject &tls, const QString &tp)
        {
            using Level = amnezia::XrayInboundIssue::Level;
            const QString serverName = tls.value(QStringLiteral("serverName")).toString().trimmed();
            if (!serverName.isEmpty() && !isValidHost(serverName, true)) {
                add(Level::Error, tp + QStringLiteral(".serverName"),
                    tr("serverName: enter a valid IP address or domain name"));
            }
            const QJsonArray alpn = tls.value(QStringLiteral("alpn")).toArray();
            for (int k = 0; k < alpn.size(); ++k) {
                const QString entry = alpn.at(k).toString();
                if (entry != QLatin1String("h2") && entry != QLatin1String("http/1.1")) {
                    add(Level::Error, tp + QStringLiteral(".alpn[%1]").arg(k),
                        tr("alpn supports only h2 and http/1.1, got \"%1\"").arg(entry));
                }
            }
        }

        void checkXhttp(const QJsonObject &x, const QString &xp, const QString &security)
        {
            using Level = amnezia::XrayInboundIssue::Level;
            const QString mode = x.value(QStringLiteral("mode")).toString(QStringLiteral("auto"));
            static const QStringList modes { QStringLiteral("auto"), QStringLiteral("packet-up"),
                                             QStringLiteral("stream-up"), QStringLiteral("stream-one") };
            if (!modes.contains(mode)) {
                add(Level::Error, xp + QStringLiteral(".mode"),
                    tr("mode must be auto, packet-up, stream-up or stream-one"));
            }
            const QString host = x.value(QStringLiteral("host")).toString().trimmed();
            if (!host.isEmpty() && !isValidHost(host)) {
                add(Level::Error, xp + QStringLiteral(".host"), tr("host: enter a valid IP address or domain name"));
            }
            const QJsonObject headers = x.value(QStringLiteral("headers")).toObject();
            for (auto it = headers.begin(); it != headers.end(); ++it) {
                if (it.key().compare(QLatin1String("host"), Qt::CaseInsensitive) == 0) {
                    add(Level::Error, xp + QStringLiteral(".headers"),
                        tr("headers must not contain Host, use the host field"));
                }
            }

            const QString method = x.value(QStringLiteral("uplinkHTTPMethod")).toString().toUpper();
            static const QStringList methods { QStringLiteral("POST"), QStringLiteral("PUT"), QStringLiteral("PATCH") };
            if (!method.isEmpty() && !methods.contains(method)
                && !(method == QLatin1String("GET") && mode == QLatin1String("packet-up"))) {
                add(Level::Error, xp + QStringLiteral(".uplinkHTTPMethod"),
                    tr("uplinkHTTPMethod must be POST, PUT or PATCH, GET works only with mode packet-up"));
            }

            const QString uplinkDataPlacement = x.value(QStringLiteral("uplinkDataPlacement")).toString();
            static const QStringList dataPlacements { QStringLiteral("body"), QStringLiteral("auto"),
                                                      QStringLiteral("header"), QStringLiteral("cookie") };
            if (!uplinkDataPlacement.isEmpty() && !dataPlacements.contains(uplinkDataPlacement)) {
                add(Level::Error, xp + QStringLiteral(".uplinkDataPlacement"),
                    tr("uplinkDataPlacement must be body, auto, header or cookie"));
            } else if ((uplinkDataPlacement == QLatin1String("header") || uplinkDataPlacement == QLatin1String("cookie"))
                       && mode != QLatin1String("packet-up")) {
                add(Level::Error, xp + QStringLiteral(".uplinkDataPlacement"),
                    tr("uplinkDataPlacement %1 works only with mode packet-up").arg(uplinkDataPlacement));
            }

            static const QStringList placements { QStringLiteral("path"), QStringLiteral("cookie"),
                                                  QStringLiteral("header"), QStringLiteral("query") };
            for (const QString &key : { QStringLiteral("sessionIDPlacement"), QStringLiteral("seqPlacement") }) {
                if (x.contains(key) && !placements.contains(x.value(key).toString())) {
                    add(Level::Error, xp + QLatin1Char('.') + key,
                        tr("%1 must be path, cookie, header or query").arg(key));
                }
            }

            static const QRegularExpression keyName(QStringLiteral(R"(^[A-Za-z0-9_-]*$)"));
            for (const QString &key :
                 { QStringLiteral("sessionIDKey"), QStringLiteral("seqKey"), QStringLiteral("uplinkDataKey"),
                   QStringLiteral("xPaddingKey"), QStringLiteral("xPaddingHeader") }) {
                if (x.contains(key) && !keyName.match(x.value(key).toString()).hasMatch()) {
                    add(Level::Error, xp + QLatin1Char('.') + key,
                        tr("%1: use only Latin letters, digits, hyphen and underscore").arg(key));
                }
            }

            {
                static const QStringList paddingPlacements { QStringLiteral("cookie"), QStringLiteral("header"),
                                                             QStringLiteral("query"), QStringLiteral("queryInHeader") };
                if (x.contains(QStringLiteral("xPaddingPlacement"))
                    && !paddingPlacements.contains(x.value(QStringLiteral("xPaddingPlacement")).toString())) {
                    add(Level::Error, xp + QStringLiteral(".xPaddingPlacement"),
                        tr("xPaddingPlacement must be cookie, header, query or queryInHeader"));
                }
                static const QStringList paddingMethods { QStringLiteral("repeat-x"), QStringLiteral("tokenish") };
                if (x.contains(QStringLiteral("xPaddingMethod"))
                    && !paddingMethods.contains(x.value(QStringLiteral("xPaddingMethod")).toString())) {
                    add(Level::Error, xp + QStringLiteral(".xPaddingMethod"),
                        tr("xPaddingMethod must be repeat-x or tokenish"));
                }
            }

            const IntRange padding = checkRange(x, xp, QStringLiteral("xPaddingBytes"), false);
            if (padding.present && padding.valid && (padding.from == 0) != (padding.to == 0)) {
                add(Level::Error, xp + QStringLiteral(".xPaddingBytes"), tr("xPaddingBytes can be 0 only as 0-0"));
            }

            checkRange(x, xp, QStringLiteral("uplinkChunkSize"), true);
            checkNumber(x, xp, QStringLiteral("scMaxBufferedPosts"), 0, 100000);

            const bool packetUp = mode == QLatin1String("packet-up")
                    || (mode == QLatin1String("auto") && security != QLatin1String("reality"));
            const IntRange postBytes = checkRange(x, xp, QStringLiteral("scMaxEachPostBytes"), false);
            if (packetUp && postBytes.present && postBytes.valid) {
                const qint64 low = qMin(postBytes.from, postBytes.to);
                const qint64 high = qMax(postBytes.from, postBytes.to);
                if (low == 0 && high > 0) {
                    add(Level::Error, xp + QStringLiteral(".scMaxEachPostBytes"),
                        tr("scMaxEachPostBytes can start at 0 only when the upper bound is 0 too"));
                } else if (low > 0 && low < 1000) {
                    add(Level::Error, xp + QStringLiteral(".scMaxEachPostBytes"),
                        tr("scMaxEachPostBytes: minimum is 1000"));
                }
            }
            checkRange(x, xp, QStringLiteral("scMinPostsIntervalMs"), true);
            const IntRange streamUpSecs = checkRange(x, xp, QStringLiteral("scStreamUpServerSecs"), true);
            if (mode == QLatin1String("stream-up") && streamUpSecs.present && streamUpSecs.valid
                && qMin(streamUpSecs.from, streamUpSecs.to) == 0 && qMax(streamUpSecs.from, streamUpSecs.to) > 0) {
                add(Level::Error, xp + QStringLiteral(".scStreamUpServerSecs"),
                    tr("scStreamUpServerSecs can start at 0 only when the upper bound is 0 too"));
            }

            const QJsonObject xmux = x.value(QStringLiteral("xmux")).toObject();
            const QString mp = xp + QStringLiteral(".xmux");
            for (const QString &key : { QStringLiteral("cMaxReuseTimes"), QStringLiteral("hMaxRequestTimes"),
                                        QStringLiteral("hMaxReusableSecs") }) {
                checkRange(xmux, mp, key, true);
            }
            checkNumber(xmux, mp, QStringLiteral("hKeepAlivePeriod"), -2147483647, 2147483647);
            const IntRange concurrency = checkRange(xmux, mp, QStringLiteral("maxConcurrency"), true);
            const IntRange connections = checkRange(xmux, mp, QStringLiteral("maxConnections"), true);
            auto isSet = [](const IntRange &range) { return range.present && (range.from != 0 || range.to != 0); };
            if (isSet(concurrency) && isSet(connections)) {
                add(Level::Error, mp, tr("xmux cannot set maxConnections and maxConcurrency together"));
            }
        }

        IntRange checkRange(const QJsonObject &parent, const QString &parentPath, const QString &key, bool allowNegative)
        {
            const IntRange range = readRange(parent.value(key), allowNegative);
            if (!range.valid) {
                add(amnezia::XrayInboundIssue::Level::Error, parentPath + QLatin1Char('.') + key,
                    allowNegative ? tr("%1 must be a whole number or a range like 10-20").arg(key)
                                  : tr("%1 must be a whole number 0 or more, or a range like 100-1000").arg(key));
            }
            return range;
        }

        qint64 checkNumber(const QJsonObject &parent, const QString &parentPath, const QString &key, qint64 low,
                           qint64 high)
        {
            if (!parent.contains(key)) {
                return -1;
            }
            const double value = parent.value(key).toDouble(-1);
            if (!parent.value(key).isDouble() || value != static_cast<qint64>(value) || value < low || value > high) {
                add(amnezia::XrayInboundIssue::Level::Error, parentPath + QLatin1Char('.') + key,
                    tr("%1: allowed range is %2–%3").arg(key).arg(low).arg(high));
                return -1;
            }
            return static_cast<qint64>(value);
        }

        void checkKcp(const QJsonObject &k, const QString &kp)
        {
            using Level = amnezia::XrayInboundIssue::Level;
            checkNumber(k, kp, QStringLiteral("tti"), 10, 100);
            const qint64 mtu = checkNumber(k, kp, QStringLiteral("mtu"), 576, 1460);
            checkNumber(k, kp, QStringLiteral("uplinkCapacity"), 0, 4095);
            checkNumber(k, kp, QStringLiteral("downlinkCapacity"), 0, 4095);
            checkNumber(k, kp, QStringLiteral("cwndMultiplier"), 1, 5000);
            const qint64 mtuEffective =
                    mtu > 0 ? mtu : QString::fromLatin1(amnezia::protocols::xray::defaultMkcpMtu).toLongLong();
            checkNumber(k, kp, QStringLiteral("maxSendingWindow"), mtuEffective, 2147483647);
            if (k.contains(QStringLiteral("seed")) || k.contains(QStringLiteral("header"))) {
                add(Level::Error, kp, tr("seed and header were removed from mKCP"));
            }
        }

        int lineOf(QString path) const
        {
            static const QRegularExpression lastSegment(QStringLiteral("(\\.[^.\\[]+|\\[\\d+\\])$"));
            while (!path.isEmpty()) {
                const auto it = m_lines.constFind(path);
                if (it != m_lines.constEnd()) {
                    return it.value();
                }
                const QString parent = QString(path).remove(lastSegment);
                if (parent == path) {
                    break;
                }
                path = parent;
            }
            return 1;
        }

        void add(amnezia::XrayInboundIssue::Level level, const QString &path, const QString &message)
        {
            amnezia::XrayInboundIssue issue;
            issue.level = level;
            issue.line = lineOf(path);
            issue.message = message;
            m_issues.append(issue);
        }

        const QJsonObject &m_inbound;
        const QHash<QString, int> &m_lines;
        QList<amnezia::XrayInboundIssue> &m_issues;
    };

    int levelRank(amnezia::XrayInboundIssue::Level level)
    {
        return static_cast<int>(level);
    }

    QString levelName(amnezia::XrayInboundIssue::Level level)
    {
        switch (level) {
        case amnezia::XrayInboundIssue::Level::Error: return QStringLiteral("error");
        case amnezia::XrayInboundIssue::Level::Info: return QStringLiteral("info");
        }
        return {};
    }
}

namespace amnezia
{

    bool XrayInboundValidation::isValid() const
    {
        return parsed && firstError() == nullptr;
    }

    const XrayInboundIssue *XrayInboundValidation::firstError() const
    {
        for (const XrayInboundIssue &issue : issues) {
            if (issue.level == XrayInboundIssue::Level::Error) {
                return &issue;
            }
        }
        return nullptr;
    }

    XrayInboundValidation validateXrayInbound(const QString &text)
    {
        XrayInboundValidation result;

        LineTrackingParser parser(text);
        const QJsonValue root = parser.parse();
        if (parser.failed()) {
            XrayInboundIssue issue;
            issue.line = parser.errorLine();
            issue.column = parser.errorColumn();
            issue.message = parser.errorMessage();
            result.issues.append(issue);
            logger.info() << "validate: syntax error at line" << QString::number(issue.line) << "column"
                          << QString::number(issue.column) << issue.message << "length" << QString::number(text.size());
            return result;
        }
        if (!root.isObject()) {
            XrayInboundIssue issue;
            issue.message = tr("the config must be a JSON object { … }");
            result.issues.append(issue);
            logger.info() << "validate: root is not an object";
            return result;
        }

        result.parsed = true;
        result.inbound = root.toObject();
        Rules(result.inbound, parser.lines(), result.issues).run();
        std::stable_sort(result.issues.begin(), result.issues.end(),
                         [](const XrayInboundIssue &a, const XrayInboundIssue &b) {
                             return levelRank(a.level) != levelRank(b.level) ? levelRank(a.level) < levelRank(b.level)
                                                                             : a.line < b.line;
                         });

        QStringList summary;
        for (const XrayInboundIssue &issue : std::as_const(result.issues)) {
            summary << QStringLiteral("%1@%2:%3").arg(levelName(issue.level)).arg(issue.line).arg(issue.message);
        }
        logger.info() << "validate: valid" << (result.isValid() ? "yes" : "no") << "issues"
                      << QString::number(result.issues.size()) << summary.join(QStringLiteral(" | "));
        return result;
    }

} // namespace amnezia

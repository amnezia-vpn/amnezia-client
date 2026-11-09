#include "telemtConfigModel.h"

#include "ui/utils/mtProxyPublicHostInput.h"

#include <QHostAddress>
#include <QRegExp>
#include <QRegularExpression>

#include "core/utils/networkUtilities.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "qrcodegen.hpp"

using namespace amnezia;

TelemtConfigModel::TelemtConfigModel(QObject *parent) : QObject(parent) {}

void TelemtConfigModel::applyDefaults(TelemtProtocolConfig &c) {
    if (c.port.isEmpty()) {
        c.port = QString::fromUtf8(protocols::telemt::defaultPort);
    }
    if (c.transportMode.isEmpty()) {
        c.transportMode = QString::fromUtf8(protocols::telemt::transportModeStandard);
    }
    if (c.workersMode.isEmpty()) {
        c.workersMode = QString::fromUtf8(protocols::telemt::workersModeAuto);
    }
    if (c.workers.isEmpty()) {
        c.workers = QString::fromUtf8(protocols::telemt::defaultWorkers);
    }
    if (c.userName.isEmpty()) {
        c.userName = QString::fromUtf8(protocols::telemt::defaultUserName);
    }
}

QString TelemtConfigModel::port() const {
    return m_protocolConfig.port.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultPort)
                                           : m_protocolConfig.port;
}

QString TelemtConfigModel::secret() const {
    return m_protocolConfig.secret;
}

QString TelemtConfigModel::tag() const {
    return m_protocolConfig.tag;
}

QString TelemtConfigModel::tgLink() const {
    return m_protocolConfig.tgLink;
}

QString TelemtConfigModel::tmeLink() const {
    return m_protocolConfig.tmeLink;
}

bool TelemtConfigModel::isEnabled() const {
    return m_protocolConfig.isEnabled;
}

QString TelemtConfigModel::publicHost() const {
    return m_protocolConfig.publicHost.isEmpty() ? m_fullConfig.value(QString(configKey::hostName)).toString()
                                                 : m_protocolConfig.publicHost;
}

QString TelemtConfigModel::transportMode() const {
    return m_protocolConfig.transportMode.isEmpty() ? QString::fromUtf8(protocols::telemt::transportModeStandard)
                                                    : m_protocolConfig.transportMode;
}

QString TelemtConfigModel::tlsDomain() const {
    return m_protocolConfig.tlsDomain;
}

QStringList TelemtConfigModel::additionalSecrets() const {
    return m_protocolConfig.additionalSecrets;
}

void TelemtConfigModel::setAdditionalSecrets(const QStringList &secrets) {
    if (m_protocolConfig.additionalSecrets == secrets) {
        return;
    }
    m_protocolConfig.additionalSecrets = secrets;
    emit changed();
}

QString TelemtConfigModel::workersMode() const {
    return m_protocolConfig.workersMode.isEmpty() ? QString::fromUtf8(protocols::telemt::workersModeAuto)
                                                  : m_protocolConfig.workersMode;
}

QString TelemtConfigModel::workers() const {
    return m_protocolConfig.workers.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultWorkers)
                                              : m_protocolConfig.workers;
}

bool TelemtConfigModel::natEnabled() const {
    return m_protocolConfig.natEnabled;
}

QString TelemtConfigModel::natInternalIp() const {
    return m_protocolConfig.natInternalIp;
}

QString TelemtConfigModel::natExternalIp() const {
    return m_protocolConfig.natExternalIp;
}

bool TelemtConfigModel::maskEnabled() const {
    return m_protocolConfig.maskEnabled;
}

bool TelemtConfigModel::useMiddleProxy() const {
    return m_protocolConfig.useMiddleProxy;
}

bool TelemtConfigModel::tlsEmulation() const {
    return m_protocolConfig.tlsEmulation;
}

QString TelemtConfigModel::userName() const {
    return m_protocolConfig.userName.isEmpty() ? QString::fromUtf8(protocols::telemt::defaultUserName)
                                               : m_protocolConfig.userName;
}

void TelemtConfigModel::updateModel(DockerContainer container, const TelemtProtocolConfig &protocolConfig) {
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaults(m_protocolConfig);
    emit changed();
}

void TelemtConfigModel::updateModel(const QJsonObject &config) {
    m_fullConfig = config;
    m_protocolConfig = TelemtProtocolConfig::fromJson(config.value(QString(configKey::telemt)).toObject());
    applyDefaults(m_protocolConfig);

    emit changed();
}

QJsonObject TelemtConfigModel::getConfig() {
    m_fullConfig.insert(QString(configKey::telemt), m_protocolConfig.toJson());
    return m_fullConfig;
}

TelemtProtocolConfig TelemtConfigModel::getProtocolConfig() {
    return m_protocolConfig;
}

void TelemtConfigModel::generateSecret() {
    QString secret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        secret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.secret = secret;
    emit changed();
}

void TelemtConfigModel::setSecret(const QString &secret) {
    if (secret.isEmpty() || m_protocolConfig.secret == secret) {
        return;
    }
    m_protocolConfig.secret = secret;
    emit changed();
}

bool TelemtConfigModel::validateAndSetSecret(const QString &rawSecret) {
    if (!QRegularExpression("^[0-9a-fA-F]{32}$").match(rawSecret).hasMatch()) {
        return false;
    }
    setSecret(rawSecret);
    return true;
}

void TelemtConfigModel::setPort(const QString &port) {
    if (m_protocolConfig.port == port) {
        return;
    }
    m_protocolConfig.port = port;
    emit changed();
}

void TelemtConfigModel::setTag(const QString &tag) {
    const QString sanitized = sanitizeMtProxyTagFieldText(tag);
    if (!isValidMtProxyTag(sanitized) || m_protocolConfig.tag == sanitized) {
        return;
    }
    m_protocolConfig.tag = sanitized;
    emit changed();
}

void TelemtConfigModel::setPublicHost(const QString &host) {
    const QString t = host.trimmed();
    if (!isValidPublicHost(t) || m_protocolConfig.publicHost == t) {
        return;
    }
    m_protocolConfig.publicHost = t;
    emit changed();
}

void TelemtConfigModel::setTransportMode(const QString &mode) {
    if (m_protocolConfig.transportMode == mode) {
        return;
    }
    m_protocolConfig.transportMode = mode;
    emit changed();
}

QString TelemtConfigModel::getTransportMode() const {
    return m_protocolConfig.transportMode.isEmpty()
           ? QString(protocols::telemt::transportModeStandard)
           : m_protocolConfig.transportMode;
}

QString TelemtConfigModel::getTlsDomain() const {
    return m_protocolConfig.tlsDomain.isEmpty()
           ? QString(protocols::telemt::defaultTlsDomain)
           : m_protocolConfig.tlsDomain;
}

QString TelemtConfigModel::getPublicHost() const {
    return m_protocolConfig.publicHost;
}

void TelemtConfigModel::setTlsDomain(const QString &domain) {
    const QString t = domain.trimmed();
    if (!isValidFakeTlsDomain(t) || m_protocolConfig.tlsDomain == t) {
        return;
    }
    m_protocolConfig.tlsDomain = t;
    emit changed();
}

void TelemtConfigModel::setWorkersMode(const QString &mode) {
    if (m_protocolConfig.workersMode == mode) {
        return;
    }
    m_protocolConfig.workersMode = mode;
    emit changed();
}

void TelemtConfigModel::setWorkers(const QString &workers) {
    if (m_protocolConfig.workers == workers) {
        return;
    }
    m_protocolConfig.workers = workers;
    emit changed();
}

void TelemtConfigModel::setNatEnabled(bool enabled) {
    if (m_protocolConfig.natEnabled == enabled) {
        return;
    }
    m_protocolConfig.natEnabled = enabled;
    emit changed();
}

void TelemtConfigModel::setNatInternalIp(const QString &ip) {
    const QString t = ip.trimmed();
    if (!isValidOptionalIpv4(t) || m_protocolConfig.natInternalIp == t) {
        return;
    }
    m_protocolConfig.natInternalIp = t;
    emit changed();
}

void TelemtConfigModel::setNatExternalIp(const QString &ip) {
    const QString t = ip.trimmed();
    if (!isValidOptionalIpv4(t) || m_protocolConfig.natExternalIp == t) {
        return;
    }
    m_protocolConfig.natExternalIp = t;
    emit changed();
}

void TelemtConfigModel::setMaskEnabled(bool enabled) {
    if (m_protocolConfig.maskEnabled == enabled) {
        return;
    }
    m_protocolConfig.maskEnabled = enabled;
    emit changed();
}

void TelemtConfigModel::setUseMiddleProxy(bool enabled) {
    if (m_protocolConfig.useMiddleProxy == enabled) {
        return;
    }
    m_protocolConfig.useMiddleProxy = enabled;
    emit changed();
}

void TelemtConfigModel::setTlsEmulation(bool enabled) {
    if (m_protocolConfig.tlsEmulation == enabled) {
        return;
    }
    m_protocolConfig.tlsEmulation = enabled;
    emit changed();
}

void TelemtConfigModel::setUserName(const QString &name) {
    if (m_protocolConfig.userName == name) {
        return;
    }
    m_protocolConfig.userName = name;
    emit changed();
}

void TelemtConfigModel::addAdditionalSecret() {
    QString newSecret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        newSecret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.additionalSecrets.append(newSecret);
    emit changed();
}

void TelemtConfigModel::removeAdditionalSecret(int idx) {
    if (idx < 0 || idx >= m_protocolConfig.additionalSecrets.size()) {
        return;
    }
    m_protocolConfig.additionalSecrets.removeAt(idx);
    emit changed();
}

QVariantList TelemtConfigModel::additionalSecretsList() const {
    QVariantList out;
    out.reserve(m_protocolConfig.additionalSecrets.size());
    for (const auto &s : m_protocolConfig.additionalSecrets) {
        if (!s.isEmpty()) {
            out.append(s);
        }
    }
    return out;
}

void TelemtConfigModel::setEnabled(bool enabled) {
    if (m_protocolConfig.isEnabled == enabled) {
        return;
    }
    m_protocolConfig.isEnabled = enabled;
    emit changed();
}

QString TelemtConfigModel::generateQrCode(const QString &text) {
    if (text.isEmpty()) {
        return "";
    }
    auto qr = qrCodeUtils::generateQrCode(text.toUtf8());
    return qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));
}

QString TelemtConfigModel::defaultTlsDomain() const {
    return protocols::telemt::defaultTlsDomain;
}

QString TelemtConfigModel::defaultPort() const {
    return protocols::telemt::defaultPort;
}

QString TelemtConfigModel::defaultWorkers() const {
    return protocols::telemt::defaultWorkers;
}

int TelemtConfigModel::maxWorkers() const {
    return protocols::telemt::maxWorkers;
}

QString TelemtConfigModel::transportModeStandard() const {
    return protocols::telemt::transportModeStandard;
}

QString TelemtConfigModel::transportModeFakeTLS() const {
    return protocols::telemt::transportModeFakeTLS;
}

QString TelemtConfigModel::workersModeAuto() const {
    return protocols::telemt::workersModeAuto;
}

QString TelemtConfigModel::workersModeManual() const {
    return protocols::telemt::workersModeManual;
}

bool TelemtConfigModel::isValidPublicHost(const QString &host) const {
    const QString t = host.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (t.length() > 253) {
        return false;
    }
    QHostAddress a(t);
    if (a.protocol() == QHostAddress::IPv4Protocol) {
        return NetworkUtilities::checkIPv4Format(t);
    }
    if (a.protocol() == QHostAddress::IPv6Protocol) {
        if (a.isNull() || a.isLoopback() || a == QHostAddress(QHostAddress::AnyIPv6)) {
            return false;
        }
        return true;
    }
    static const QRegularExpression onlyAsciiDigits(QStringLiteral(R"(^\d+$)"));
    if (onlyAsciiDigits.match(t).hasMatch()) {
        return false;
    }
    return NetworkUtilities::domainRegExp().exactMatch(t);
}

bool TelemtConfigModel::isPublicHostInputAllowed(const QString &text) const {
    return mtproxyPublicHostInputAllowed(text);
}

bool TelemtConfigModel::isPublicHostTypingIncomplete(const QString &text) const {
    const QString t = text.trimmed();
    if (isValidPublicHost(t)) {
        return false;
    }

    static const QRegularExpression onlyDigitDot(QStringLiteral(R"(^[0-9.]+$)"));
    if (onlyDigitDot.match(t).hasMatch()) {
        if (t.endsWith(QLatin1Char('.'))) {
            return true;
        }
        const QStringList parts = t.split(QLatin1Char('.'), Qt::KeepEmptyParts);
        if (parts.size() < 4) {
            return true;
        }
        for (const QString &part: parts) {
            if (part.isEmpty()) {
                return true;
            }
        }
        return false;
    }

    if (t.contains(QLatin1Char(':'))) {
        if (t.contains(QLatin1String(":::"))) {
            return false;
        }
        if (t.endsWith(QLatin1Char(':'))) {
            return true;
        }
        QHostAddress a(t);
        if (a.protocol() == QHostAddress::IPv6Protocol) {
            return false;
        }
        if (!t.contains(QLatin1String("::")) && t.count(QLatin1Char(':')) < 7 && !t.contains(QLatin1Char('.'))) {
            return true;
        }
        return false;
    }

    if (!t.contains(QLatin1Char('.'))) {
        return true;
    }

    return false;
}

bool TelemtConfigModel::isValidMtProxyTag(const QString &tag) const {
    if (tag.isEmpty()) {
        return true;
    }
    static const QRegularExpression re(
            QStringLiteral("^([0-9a-fA-F]{%1})$").arg(protocols::telemt::botTagHexLength));
    return re.match(tag).hasMatch();
}

bool TelemtConfigModel::isMtProxyTagTypingIncomplete(const QString &text) const {
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    static const QRegularExpression hexOnly(QStringLiteral(R"(^[0-9a-fA-F]*$)"));
    if (!hexOnly.match(t).hasMatch()) {
        return false;
    }
    return t.size() < protocols::telemt::botTagHexLength;
}

int TelemtConfigModel::mtProxyBotTagHexLength() const {
    return protocols::telemt::botTagHexLength;
}

bool TelemtConfigModel::isValidFakeTlsDomain(const QString &domain) const {
    const QString t = domain.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (t.length() > 253) {
        return false;
    }
    QHostAddress addr;
    if (addr.setAddress(t)) {
        return false;
    }
    static const QRegularExpression onlyAsciiDigits(QStringLiteral(R"(^\d+$)"));
    if (onlyAsciiDigits.match(t).hasMatch()) {
        return false;
    }
    QRegExp re(NetworkUtilities::domainRegExp());
    re.setCaseSensitivity(Qt::CaseInsensitive);
    if (!re.exactMatch(t)) {
        return false;
    }
    if (t.toUtf8().size() > 111) {
        return false;
    }
    return true;
}

QString TelemtConfigModel::normalizeFakeTlsDomainInput(const QString &input) const {
    QString t = input.trimmed();
    if (t.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        t = t.mid(8);
    } else if (t.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)) {
        t = t.mid(7);
    }
    if (const int slash = t.indexOf(QLatin1Char('/')); slash >= 0) {
        t = t.left(slash);
    }
    if (const int at = t.indexOf(QLatin1Char('@')); at >= 0) {
        t = t.mid(at + 1);
    }
    if (const int colon = t.indexOf(QLatin1Char(':')); colon >= 0) {
        t = t.left(colon);
    }
    if (t.startsWith(QLatin1String("www."), Qt::CaseInsensitive)) {
        const QString rest = t.mid(4);
        if (rest.contains(QLatin1Char('.'))) {
            t = rest;
        }
    }
    return t.trimmed();
}

bool TelemtConfigModel::isFakeTlsDomainTypingIncomplete(const QString &text) const {
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (isValidFakeTlsDomain(t)) {
        return false;
    }
    if (t.contains(QLatin1Char('/')) || t.contains(QLatin1Char(':')) || t.contains(QLatin1Char('@'))
        || t.contains(QLatin1Char(' '))) {
        return false;
    }
    if (t.contains(QLatin1String(".."))) {
        return false;
    }
    if (!t.contains(QLatin1Char('.'))) {
        return true;
    }
    if (t.endsWith(QLatin1Char('.'))) {
        return true;
    }
    static const QRegularExpression legalPartial(QStringLiteral(R"(^[a-zA-Z0-9.-]*$)"));
    if (!legalPartial.match(t).hasMatch()) {
        return false;
    }
    return true;
}

bool TelemtConfigModel::isFakeTlsDomainInputAllowed(const QString &text) const {
    if (text.length() > 253) {
        return false;
    }
    static const QRegularExpression re(QStringLiteral(R"(^[a-zA-Z0-9.-]*$)"));
    return re.match(text).hasMatch();
}

QString TelemtConfigModel::sanitizeFakeTlsDomainFieldText(const QString &input) const {
    const QString t = normalizeFakeTlsDomainInput(input);
    QString out;
    out.reserve(t.size());
    for (const QChar &c: t) {
        const ushort u = c.unicode();
        const bool letter = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
        const bool digit = (u >= '0' && u <= '9');
        if (letter || digit || u == '.' || u == '-') {
            out.append(c);
        }
    }
    if (out.size() > 253) {
        out.truncate(253);
    }
    return out;
}

QString TelemtConfigModel::sanitizePublicHostFieldText(const QString &input) const {
    QString out;
    const int cap = qMin(input.size(), 253);
    out.reserve(cap);
    for (const QChar &c: input) {
        if (out.size() >= 253) {
            break;
        }
        const ushort u = c.unicode();
        if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '.' || u == ':' ||
            u == '-') {
            out.append(c);
        }
    }
    return out;
}

QString TelemtConfigModel::sanitizePortFieldText(const QString &input) const {
    QString out;
    out.reserve(qMin(input.size(), 5));
    for (const QChar &c: input) {
        const ushort u = c.unicode();
        if (u >= '0' && u <= '9' && out.size() < 5) {
            out.append(c);
        }
    }
    return out;
}

QString TelemtConfigModel::sanitizeMtProxyTagFieldText(const QString &input) const {
    QString trimmed = input.trimmed();
    if (trimmed.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        trimmed = trimmed.mid(2).trimmed();
    }
    static const QRegularExpression runHex(QStringLiteral(R"(([0-9a-fA-F]{32}))"));
    const QRegularExpressionMatch m = runHex.match(trimmed);
    if (m.hasMatch()) {
        return m.captured(1);
    }
    const int cap = protocols::telemt::botTagHexLength;
    QString out;
    out.reserve(qMin(trimmed.size(), cap));
    for (const QChar &c: trimmed) {
        if (out.size() >= cap) {
            break;
        }
        const ushort u = c.unicode();
        if ((u >= '0' && u <= '9') || (u >= 'a' && u <= 'f') || (u >= 'A' && u <= 'F')) {
            out.append(c);
        }
    }
    return out;
}

QString TelemtConfigModel::sanitizeOptionalIpv4FieldText(const QString &input) const {
    QString out;
    out.reserve(qMin(input.size(), 15));
    for (const QChar &c: input) {
        if (out.size() >= 15) {
            break;
        }
        const ushort u = c.unicode();
        if ((u >= '0' && u <= '9') || u == '.') {
            out.append(c);
        }
    }
    return out;
}

bool TelemtConfigModel::isValidOptionalIpv4(const QString &ip) const {
    const QString t = ip.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    return NetworkUtilities::checkIPv4Format(t);
}

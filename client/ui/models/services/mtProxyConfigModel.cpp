#include "mtProxyConfigModel.h"

#include "ui/models/utils/mtproxy_public_host_input.h"

#include "core/utils/networkUtilities.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/constants/configKeys.h"
#include "qrcodegen.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QHostAddress>
#include <QRegExp>
#include <QRegularExpression>
#include <QtGlobal>
#include <qqml.h>

using namespace amnezia;

MtProxyConfigModel::MtProxyConfigModel(QObject *parent) : QAbstractListModel(parent) {
    qmlRegisterType<PublicHostInputValidator>("MtProxyConfig", 1, 0, "PublicHostInputValidator");
}

int MtProxyConfigModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 1;
}

bool MtProxyConfigModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() != 0) {
        return false;
    }

    switch (role) {
        case Roles::PortRole: {
            m_protocolConfig.port = value.toString();
            break;
        }
        case Roles::SecretRole: {
            m_protocolConfig.secret = value.toString();
            break;
        }
        case Roles::TagRole: {
            const QString tag = sanitizeMtProxyTagFieldText(value.toString());
            if (!isValidMtProxyTag(tag)) {
                return false;
            }
            m_protocolConfig.tag = tag;
            break;
        }
        case Roles::IsEnabledRole: {
            m_protocolConfig.isEnabled = value.toBool();
            break;
        }
        case Roles::PublicHostRole: {
            const QString h = value.toString().trimmed();
            if (!isValidPublicHost(h)) {
                return false;
            }
            m_protocolConfig.publicHost = h;
            break;
        }
        case Roles::TransportModeRole: {
            m_protocolConfig.transportMode = value.toString();
            break;
        }
        case Roles::TlsDomainRole: {
            const QString d = value.toString().trimmed();
            if (!isValidFakeTlsDomain(d)) {
                return false;
            }
            m_protocolConfig.tlsDomain = d;
            break;
        }
        case Roles::AdditionalSecretsRole: {
            m_protocolConfig.additionalSecrets = value.toStringList();
            break;
        }
        case Roles::WorkersModeRole: {
            m_protocolConfig.workersMode = value.toString();
            break;
        }
        case Roles::WorkersRole: {
            m_protocolConfig.workers = value.toString();
            break;
        }
        case Roles::NatEnabledRole: {
            m_protocolConfig.natEnabled = value.toBool();
            break;
        }
        case Roles::NatInternalIpRole: {
            const QString ip = value.toString().trimmed();
            if (!isValidOptionalIpv4(ip)) {
                return false;
            }
            m_protocolConfig.natInternalIp = ip;
            break;
        }
        case Roles::NatExternalIpRole: {
            const QString ip = value.toString().trimmed();
            if (!isValidOptionalIpv4(ip)) {
                return false;
            }
            m_protocolConfig.natExternalIp = ip;
            break;
        }
        default: {
            return false;
        }
    }

    emit dataChanged(index, index, QList{role});
    return true;
}

QVariant MtProxyConfigModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() != 0) {
        return QVariant();
    }

    switch (role) {
        case Roles::PortRole: {
            return m_protocolConfig.port.isEmpty() ? QString(protocols::mtProxy::defaultPort) : m_protocolConfig.port;
        }
        case Roles::SecretRole: {
            return m_protocolConfig.secret;
        }
        case Roles::TagRole: {
            return m_protocolConfig.tag;
        }
        case Roles::TgLinkRole: {
            return m_protocolConfig.tgLink;
        }
        case Roles::TmeLinkRole: {
            return m_protocolConfig.tmeLink;
        }
        case Roles::IsEnabledRole: {
            return m_protocolConfig.isEnabled;
        }
        case Roles::PublicHostRole: {
            return m_protocolConfig.publicHost.isEmpty()
                   ? m_fullConfig.value(configKey::hostName).toString()
                   : m_protocolConfig.publicHost;
        }
        case Roles::TransportModeRole: {
            return m_protocolConfig.transportMode.isEmpty()
                   ? QString(protocols::mtProxy::transportModeStandard)
                   : m_protocolConfig.transportMode;
        }
        case Roles::TlsDomainRole: {
            return m_protocolConfig.tlsDomain;
        }
        case Roles::AdditionalSecretsRole: {
            return m_protocolConfig.additionalSecrets;
        }
        case Roles::WorkersModeRole: {
            return m_protocolConfig.workersMode.isEmpty()
                   ? QString(protocols::mtProxy::workersModeAuto)
                   : m_protocolConfig.workersMode;
        }
        case Roles::WorkersRole: {
            return m_protocolConfig.workers.isEmpty() ? QString(protocols::mtProxy::defaultWorkers)
                                                      : m_protocolConfig.workers;
        }
        case Roles::NatEnabledRole: {
            return m_protocolConfig.natEnabled;
        }
        case Roles::NatInternalIpRole: {
            return m_protocolConfig.natInternalIp;
        }
        case Roles::NatExternalIpRole: {
            return m_protocolConfig.natExternalIp;
        }
    }


    return QVariant();
}

void MtProxyConfigModel::updateModel(amnezia::DockerContainer container,
                                     const amnezia::MtProxyProtocolConfig &protocolConfig) {
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    endResetModel();
}

void MtProxyConfigModel::updateModel(const QJsonObject &config) {
    beginResetModel();

    m_fullConfig = config;
    m_protocolConfig = MtProxyProtocolConfig::fromJson(config.value(configKey::mtproxy).toObject());
    if (m_protocolConfig.port.isEmpty()) m_protocolConfig.port = protocols::mtProxy::defaultPort;
    if (m_protocolConfig.transportMode.isEmpty()) m_protocolConfig.transportMode = protocols::mtProxy::transportModeStandard;
    if (m_protocolConfig.workersMode.isEmpty()) m_protocolConfig.workersMode = protocols::mtProxy::workersModeAuto;
    if (m_protocolConfig.workers.isEmpty()) m_protocolConfig.workers = protocols::mtProxy::defaultWorkers;
    {
        QString tagIn = sanitizeMtProxyTagFieldText(m_protocolConfig.tag);
        if (!isValidMtProxyTag(tagIn)) {
            tagIn.clear();
        }
        m_protocolConfig.tag = tagIn;
    }

    endResetModel();
}

QJsonObject MtProxyConfigModel::getConfig() {
    m_fullConfig.insert(configKey::mtproxy, m_protocolConfig.toJson());
    return m_fullConfig;
}

void MtProxyConfigModel::generateSecret() {
    // Generate 16 random bytes = 32 hex chars
    QString secret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        secret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.secret = secret;
    emit dataChanged(index(0), index(0), QList<int>{SecretRole});
}

void MtProxyConfigModel::setSecret(const QString &secret) {
    if (secret.isEmpty()) {
        return;
    }
    setData(index(0), secret, SecretRole);
}

bool MtProxyConfigModel::validateAndSetSecret(const QString &rawSecret) {
    if (!QRegularExpression("^[0-9a-fA-F]{32}$").match(rawSecret).hasMatch()) {
        return false;
    }
    setData(index(0), rawSecret, SecretRole);
    return true;
}

void MtProxyConfigModel::setPort(const QString &port) {
    setData(index(0), port, PortRole);
}

void MtProxyConfigModel::setTag(const QString &tag) {
    setData(index(0), tag, TagRole);
}

void MtProxyConfigModel::setPublicHost(const QString &host) {
    const QString t = host.trimmed();
    if (!isValidPublicHost(t)) {
        return;
    }
    setData(index(0), t, PublicHostRole);
}

void MtProxyConfigModel::setTransportMode(const QString &mode) {
    setData(index(0), mode, TransportModeRole);
}

QString MtProxyConfigModel::getTransportMode() const {
    return m_protocolConfig.transportMode.isEmpty()
           ? QString(protocols::mtProxy::transportModeStandard)
           : m_protocolConfig.transportMode;
}

QString MtProxyConfigModel::getTlsDomain() const {
    return m_protocolConfig.tlsDomain.isEmpty()
           ? QString(protocols::mtProxy::defaultTlsDomain)
           : m_protocolConfig.tlsDomain;
}

QString MtProxyConfigModel::getPublicHost() const {
    return m_protocolConfig.publicHost;
}

void MtProxyConfigModel::setTlsDomain(const QString &domain) {
    const QString t = domain.trimmed();
    if (!isValidFakeTlsDomain(t)) {
        return;
    }
    setData(index(0), t, TlsDomainRole);
}

void MtProxyConfigModel::setWorkersMode(const QString &mode) {
    setData(index(0), mode, WorkersModeRole);
}

void MtProxyConfigModel::setWorkers(const QString &workers) {
    setData(index(0), workers, WorkersRole);
}

void MtProxyConfigModel::setNatEnabled(bool enabled) {
    setData(index(0), enabled, NatEnabledRole);
}

void MtProxyConfigModel::setNatInternalIp(const QString &ip) {
    const QString t = ip.trimmed();
    if (!isValidOptionalIpv4(t)) {
        return;
    }
    setData(index(0), t, NatInternalIpRole);
}

void MtProxyConfigModel::setNatExternalIp(const QString &ip) {
    const QString t = ip.trimmed();
    if (!isValidOptionalIpv4(t)) {
        return;
    }
    setData(index(0), t, NatExternalIpRole);
}

void MtProxyConfigModel::addAdditionalSecret() {
    QString newSecret;
    for (int i = 0; i < 16; ++i) {
        quint32 byte = QRandomGenerator::global()->bounded(256);
        newSecret += QString("%1").arg(byte, 2, 16, QChar('0'));
    }

    m_protocolConfig.additionalSecrets.append(newSecret);
    emit dataChanged(index(0), index(0), QList<int>{AdditionalSecretsRole});
}

void MtProxyConfigModel::removeAdditionalSecret(int idx) {
    if (idx < 0 || idx >= m_protocolConfig.additionalSecrets.size()) {
        return;
    }
    m_protocolConfig.additionalSecrets.removeAt(idx);
    emit dataChanged(index(0), index(0), QList<int>{AdditionalSecretsRole});
}

QVariantList MtProxyConfigModel::additionalSecretsList() const {
    QVariantList out;
    out.reserve(m_protocolConfig.additionalSecrets.size());
    for (const auto &s: m_protocolConfig.additionalSecrets) {
        if (!s.isEmpty()) {
            out.append(s);
        }
    }
    return out;
}

void MtProxyConfigModel::setEnabled(bool enabled) {
    m_protocolConfig.isEnabled = enabled;
    emit dataChanged(index(0), index(0), QList<int>{IsEnabledRole});
}

QString MtProxyConfigModel::generateQrCode(const QString &text) {
    if (text.isEmpty()) {
        return "";
    }
    auto qr = qrCodeUtils::generateQrCode(text.toUtf8());
    return qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));
}

QString MtProxyConfigModel::defaultTlsDomain() const {
    return protocols::mtProxy::defaultTlsDomain;
}

QString MtProxyConfigModel::defaultPort() const {
    return protocols::mtProxy::defaultPort;
}

QString MtProxyConfigModel::defaultWorkers() const {
    return protocols::mtProxy::defaultWorkers;
}

int MtProxyConfigModel::maxWorkers() const {
    return protocols::mtProxy::maxWorkers;
}

QString MtProxyConfigModel::transportModeStandard() const {
    return protocols::mtProxy::transportModeStandard;
}

QString MtProxyConfigModel::transportModeFakeTLS() const {
    return protocols::mtProxy::transportModeFakeTLS;
}

QString MtProxyConfigModel::workersModeAuto() const {
    return protocols::mtProxy::workersModeAuto;
}

QString MtProxyConfigModel::workersModeManual() const {
    return protocols::mtProxy::workersModeManual;
}

bool MtProxyConfigModel::isValidPublicHost(const QString &host) const {
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
        return true;
    }
    static const QRegularExpression onlyAsciiDigits(QStringLiteral(R"(^\d+$)"));
    if (onlyAsciiDigits.match(t).hasMatch()) {
        return false;
    }
    return NetworkUtilities::domainRegExp().exactMatch(t);
}

bool MtProxyConfigModel::isPublicHostInputAllowed(const QString &text) const {
    return mtproxyPublicHostInputAllowed(text);
}

bool MtProxyConfigModel::isPublicHostTypingIncomplete(const QString &text) const {
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

bool MtProxyConfigModel::isValidMtProxyTag(const QString &tag) const {
    if (tag.isEmpty()) {
        return true;
    }
    static const QRegularExpression re(
            QStringLiteral("^([0-9a-fA-F]{%1})$").arg(protocols::mtProxy::botTagHexLength));
    return re.match(tag).hasMatch();
}

bool MtProxyConfigModel::isMtProxyTagTypingIncomplete(const QString &text) const {
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    static const QRegularExpression hexOnly(QStringLiteral(R"(^[0-9a-fA-F]*$)"));
    if (!hexOnly.match(t).hasMatch()) {
        return false;
    }
    return t.size() < protocols::mtProxy::botTagHexLength;
}

int MtProxyConfigModel::mtProxyBotTagHexLength() const {
    return protocols::mtProxy::botTagHexLength;
}

bool MtProxyConfigModel::isValidFakeTlsDomain(const QString &domain) const {
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
    // ee + 32 hex (base secret) + hex(UTF-8 domain); keep headroom under typical client limits.
    if (t.toUtf8().size() > 111) {
        return false;
    }
    return true;
}

QString MtProxyConfigModel::clipboardText() const {
    if (QClipboard *c = QGuiApplication::clipboard()) {
        return c->text();
    }
    return QString();
}

QString MtProxyConfigModel::sanitizeFakeTlsDomainFieldText(const QString &input) const {
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

bool MtProxyConfigModel::isFakeTlsDomainInputAllowed(const QString &text) const {
    if (text.length() > 253) {
        return false;
    }
    static const QRegularExpression re(QStringLiteral(R"(^[a-zA-Z0-9.-]*$)"));
    return re.match(text).hasMatch();
}

QString MtProxyConfigModel::sanitizePublicHostFieldText(const QString &input) const {
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

QString MtProxyConfigModel::sanitizePortFieldText(const QString &input) const {
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

QString MtProxyConfigModel::sanitizeMtProxyTagFieldText(const QString &input) const {
    QString trimmed = input.trimmed();
    if (trimmed.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        trimmed = trimmed.mid(2).trimmed();
    }
    // Prefer a contiguous 32-hex run (paste from bot message with extra text).
    static const QRegularExpression runHex(QStringLiteral(R"(([0-9a-fA-F]{32}))"));
    const QRegularExpressionMatch m = runHex.match(trimmed);
    if (m.hasMatch()) {
        return m.captured(1);
    }
    const int cap = protocols::mtProxy::botTagHexLength;
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

QString MtProxyConfigModel::sanitizeWorkersFieldText(const QString &input) const {
    QString out;
    out.reserve(qMin(input.size(), 3));
    for (const QChar &c: input) {
        const ushort u = c.unicode();
        if (u >= '0' && u <= '9' && out.size() < 3) {
            out.append(c);
        }
    }
    return out;
}

QString MtProxyConfigModel::sanitizeOptionalIpv4FieldText(const QString &input) const {
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

QString MtProxyConfigModel::normalizeFakeTlsDomainInput(const QString &input) const {
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

bool MtProxyConfigModel::isFakeTlsDomainTypingIncomplete(const QString &text) const {
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

bool MtProxyConfigModel::isValidOptionalIpv4(const QString &ip) const {
    const QString t = ip.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    return NetworkUtilities::checkIPv4Format(t);
}

QHash<int, QByteArray> MtProxyConfigModel::roleNames() const {
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[SecretRole] = "secret";
    roles[TagRole] = "tag";
    roles[TgLinkRole] = "tgLink";
    roles[TmeLinkRole] = "tmeLink";
    roles[IsEnabledRole] = "isEnabled";
    roles[PublicHostRole] = "publicHost";
    roles[TransportModeRole] = "transportMode";
    roles[TlsDomainRole] = "tlsDomain";
    roles[AdditionalSecretsRole] = "additionalSecrets";
    roles[WorkersModeRole] = "workersMode";
    roles[WorkersRole] = "workers";
    roles[NatEnabledRole] = "natEnabled";
    roles[NatInternalIpRole] = "natInternalIp";
    roles[NatExternalIpRole] = "natExternalIp";

    return roles;
}

amnezia::MtProxyProtocolConfig MtProxyConfigModel::getProtocolConfig() {
    return m_protocolConfig;
}

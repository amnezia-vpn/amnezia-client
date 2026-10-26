#include "tProxyConfigModel.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

#include <QRandomGenerator>
#include <QRegularExpression>

using namespace amnezia;

namespace {
    const QRegularExpression kHostnameRe(QStringLiteral("^[a-z0-9]([a-z0-9.-]*[a-z0-9])?$"));
    const QRegularExpression kEmailRe(QStringLiteral("^[A-Za-z0-9._+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"));
    const QRegularExpression kHex32Re(QStringLiteral("^[0-9a-fA-F]{32}$"));
}

TProxyConfigModel::TProxyConfigModel(QObject *parent) : QObject(parent) {}

QString TProxyConfigModel::tgLink() const
{
    return m_protocolConfig.tgLink;
}

QString TProxyConfigModel::tmeLink() const
{
    return m_protocolConfig.tmeLink;
}

bool TProxyConfigModel::isEnabled() const
{
    return m_protocolConfig.isEnabled;
}

void TProxyConfigModel::updateModel(DockerContainer container, const TProxyProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    emit changed();
}

void TProxyConfigModel::updateModel(const QJsonObject &config)
{
    m_fullConfig = config;
    m_protocolConfig = TProxyProtocolConfig::fromJson(config.value(configKey::tproxy).toObject());
    if (m_protocolConfig.port.isEmpty()) {
        m_protocolConfig.port = protocols::tProxy::defaultPort;
    }
    if (m_protocolConfig.httpPort.isEmpty()) {
        m_protocolConfig.httpPort = protocols::tProxy::defaultHttpPort;
    }
    if (m_protocolConfig.carrierMode.isEmpty()) {
        m_protocolConfig.carrierMode = protocols::tProxy::carrierModeHttps;
    }
    if (m_protocolConfig.workers.isEmpty()) {
        m_protocolConfig.workers = protocols::tProxy::defaultWorkers;
    }
    emit changed();
}

QJsonObject TProxyConfigModel::getConfig()
{
    m_fullConfig.insert(configKey::tproxy, m_protocolConfig.toJson());
    return m_fullConfig;
}

TProxyProtocolConfig TProxyConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

void TProxyConfigModel::generateSecret()
{
    QString secret;
    for (int i = 0; i < 16; ++i) {
        const quint32 byte = QRandomGenerator::global()->bounded(256);
        secret += QStringLiteral("%1").arg(byte, 2, 16, QChar('0'));
    }
    m_protocolConfig.secret = secret;
    emit changed();
}

void TProxyConfigModel::setSecret(const QString &secret)
{
    if (secret.isEmpty() || m_protocolConfig.secret == secret) {
        return;
    }
    m_protocolConfig.secret = secret;
    emit changed();
}

bool TProxyConfigModel::validateAndSetSecret(const QString &rawSecret)
{
    if (!kHex32Re.match(rawSecret).hasMatch()) {
        return false;
    }
    if (m_protocolConfig.secret != rawSecret) {
        m_protocolConfig.secret = rawSecret;
        emit changed();
    }
    return true;
}

void TProxyConfigModel::setPort(const QString &port)
{
    const QString p = sanitizePortFieldText(port);
    if (m_protocolConfig.port == p) {
        return;
    }
    m_protocolConfig.port = p;
    emit changed();
}

void TProxyConfigModel::setHttpPort(const QString &port)
{
    const QString p = sanitizePortFieldText(port);
    if (m_protocolConfig.httpPort == p) {
        return;
    }
    m_protocolConfig.httpPort = p;
    emit changed();
}

void TProxyConfigModel::setHostname(const QString &hostname)
{
    const QString h = sanitizeHostnameFieldText(hostname);
    if (!isValidHostname(h) || m_protocolConfig.hostname == h) {
        return;
    }
    m_protocolConfig.hostname = h;
    emit changed();
}

void TProxyConfigModel::setAcmeEmail(const QString &email)
{
    const QString e = sanitizeAcmeEmailFieldText(email);
    if (!isValidAcmeEmail(e) || m_protocolConfig.acmeEmail == e) {
        return;
    }
    m_protocolConfig.acmeEmail = e;
    emit changed();
}

void TProxyConfigModel::setCarrierMode(const QString &mode)
{
    if (!isValidCarrierMode(mode) || m_protocolConfig.carrierMode == mode) {
        return;
    }
    m_protocolConfig.carrierMode = mode;
    emit changed();
}

void TProxyConfigModel::setWorkers(const QString &workers)
{
    const QString w = sanitizeWorkersFieldText(workers);
    if (m_protocolConfig.workers == w) {
        return;
    }
    m_protocolConfig.workers = w;
    emit changed();
}

void TProxyConfigModel::setEnabled(bool enabled)
{
    if (m_protocolConfig.isEnabled == enabled) {
        return;
    }
    m_protocolConfig.isEnabled = enabled;
    emit changed();
}

QString TProxyConfigModel::getHostname() const
{
    return m_protocolConfig.hostname;
}

QString TProxyConfigModel::getSecret() const
{
    return m_protocolConfig.secret;
}

QString TProxyConfigModel::getAcmeEmail() const
{
    return m_protocolConfig.acmeEmail;
}

QString TProxyConfigModel::getCarrierMode() const
{
    return m_protocolConfig.carrierMode.isEmpty() ? QString(protocols::tProxy::carrierModeHttps)
                                                  : m_protocolConfig.carrierMode;
}

QString TProxyConfigModel::getWorkers() const
{
    return m_protocolConfig.workers.isEmpty() ? QString(protocols::tProxy::defaultWorkers) : m_protocolConfig.workers;
}

QString TProxyConfigModel::getPort() const
{
    return m_protocolConfig.port.isEmpty() ? QString(protocols::tProxy::defaultPort) : m_protocolConfig.port;
}

QString TProxyConfigModel::getHttpPort() const
{
    return m_protocolConfig.httpPort.isEmpty() ? QString(protocols::tProxy::defaultHttpPort) : m_protocolConfig.httpPort;
}

QString TProxyConfigModel::defaultPort() const
{
    return QString(protocols::tProxy::defaultPort);
}

QString TProxyConfigModel::defaultHttpPort() const
{
    return QString(protocols::tProxy::defaultHttpPort);
}

QString TProxyConfigModel::defaultWorkers() const
{
    return QString(protocols::tProxy::defaultWorkers);
}

int TProxyConfigModel::maxWorkers() const
{
    return protocols::tProxy::maxWorkers;
}

QString TProxyConfigModel::carrierModeHttps() const
{
    return QString(protocols::tProxy::carrierModeHttps);
}

bool TProxyConfigModel::isValidHostname(const QString &host) const
{
    if (host.isEmpty() || !host.contains(QLatin1Char('.'))) {
        return false;
    }
    return kHostnameRe.match(host).hasMatch();
}

bool TProxyConfigModel::isHostnameTypingIncomplete(const QString &text) const
{
    const QString t = text.trimmed().toLower();
    if (t.isEmpty()) {
        return true;
    }
    if (t.startsWith(QLatin1Char('-')) || t.startsWith(QLatin1Char('.'))) {
        return true;
    }
    if (t.endsWith(QLatin1Char('.')) || t.endsWith(QLatin1Char('-'))) {
        return true;
    }
    return !t.contains(QLatin1Char('.'));
}

QString TProxyConfigModel::sanitizeHostnameFieldText(const QString &input) const
{
    QString out = input.trimmed().toLower();
    out.remove(QRegularExpression(QStringLiteral("[^a-z0-9.-]")));
    while (out.startsWith(QLatin1Char('-')) || out.startsWith(QLatin1Char('.'))) {
        out.remove(0, 1);
    }
    return out;
}

bool TProxyConfigModel::isValidAcmeEmail(const QString &email) const
{
    return kEmailRe.match(email.trimmed()).hasMatch();
}

bool TProxyConfigModel::isAcmeEmailTypingIncomplete(const QString &text) const
{
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (t.endsWith(QLatin1Char('.')) || t.endsWith(QLatin1Char('@')) || t.endsWith(QLatin1Char('-'))
        || t.endsWith(QLatin1Char('+')) || t.endsWith(QLatin1Char('_'))) {
        return true;
    }
    if (!t.contains(QLatin1Char('@'))) {
        return true;
    }
    const int at = t.indexOf(QLatin1Char('@'));
    const QString domain = t.mid(at + 1);
    if (!domain.contains(QLatin1Char('.'))) {
        return true;
    }
    return false;
}

QString TProxyConfigModel::sanitizeAcmeEmailFieldText(const QString &input) const
{
    QString out;
    for (const QChar c : input.trimmed()) {
        const ushort u = c.unicode();
        const bool asciiAlnum = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9');
        if (asciiAlnum || c == QLatin1Char('.') || c == QLatin1Char('_') || c == QLatin1Char('+')
            || c == QLatin1Char('-') || c == QLatin1Char('@')) {
            out.append(c);
        }
    }
    const int firstAt = out.indexOf(QLatin1Char('@'));
    if (firstAt >= 0) {
        const int secondAt = out.indexOf(QLatin1Char('@'), firstAt + 1);
        if (secondAt >= 0) {
            out.remove(secondAt, 1);
        }
    }
    return out;
}

bool TProxyConfigModel::isValidCarrierMode(const QString &mode) const
{
    return mode == QLatin1String(protocols::tProxy::carrierModeHttps)
            || mode == QLatin1String(protocols::tProxy::carrierModeHttpsLanes)
            || mode == QLatin1String(protocols::tProxy::carrierModeWebsocket)
            || mode == QLatin1String(protocols::tProxy::carrierModeWebsocketLanes);
}

QString TProxyConfigModel::carrierModeLabel(const QString &mode) const
{
    if (mode == QLatin1String(protocols::tProxy::carrierModeHttpsLanes)) {
        return QStringLiteral("HTTPS lanes");
    }
    if (mode == QLatin1String(protocols::tProxy::carrierModeWebsocket)) {
        return QStringLiteral("WebSocket");
    }
    if (mode == QLatin1String(protocols::tProxy::carrierModeWebsocketLanes)) {
        return QStringLiteral("WebSocket lanes");
    }
    return QStringLiteral("HTTPS");
}

QString TProxyConfigModel::sanitizeWorkersFieldText(const QString &input) const
{
    QString digits;
    for (const QChar c : input) {
        if (c.isDigit()) {
            digits.append(c);
        }
    }
    bool ok = false;
    const int n = digits.toInt(&ok);
    if (!ok || n < 1) {
        return QString(protocols::tProxy::defaultWorkers);
    }
    if (n > protocols::tProxy::maxWorkers) {
        return QString::number(protocols::tProxy::maxWorkers);
    }
    return QString::number(n);
}

QString TProxyConfigModel::sanitizePortFieldText(const QString &input) const
{
    QString out;
    out.reserve(qMin(input.size(), 5));
    for (const QChar &c : input) {
        const ushort u = c.unicode();
        if (u >= '0' && u <= '9' && out.size() < 5) {
            out.append(c);
        }
    }
    return out;
}

#include "whatsAppProxyConfigModel.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"

#include <QHostAddress>
#include <QRegularExpression>

using namespace amnezia;

namespace
{
    const QRegularExpression kHostnameRe(
            QStringLiteral("^[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?(\\.[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?)+$"));
}

WhatsAppProxyConfigModel::WhatsAppProxyConfigModel(QObject *parent) : QObject(parent)
{
}

bool WhatsAppProxyConfigModel::isEnabled() const
{
    return m_protocolConfig.isEnabled;
}

void WhatsAppProxyConfigModel::updateModel(DockerContainer container, const WhatsAppProxyProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    emit changed();
}

void WhatsAppProxyConfigModel::updateModel(const QJsonObject &config)
{
    m_fullConfig = config;
    m_protocolConfig = WhatsAppProxyProtocolConfig::fromJson(config.value(configKey::whatsappproxy).toObject());
    m_protocolConfig.port = m_protocolConfig.effectivePort();
    m_protocolConfig.mediaPort = m_protocolConfig.effectiveMediaPort();
    emit changed();
}

QJsonObject WhatsAppProxyConfigModel::getConfig()
{
    m_fullConfig.insert(configKey::whatsappproxy, m_protocolConfig.toJson());
    return m_fullConfig;
}

WhatsAppProxyProtocolConfig WhatsAppProxyConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

void WhatsAppProxyConfigModel::setPort(const QString &port)
{
    const QString p = sanitizePortFieldText(port);
    if (m_protocolConfig.port == p) {
        return;
    }
    m_protocolConfig.port = p;
    emit changed();
}

void WhatsAppProxyConfigModel::setMediaPort(const QString &port)
{
    const QString p = sanitizePortFieldText(port);
    if (m_protocolConfig.mediaPort == p) {
        return;
    }
    m_protocolConfig.mediaPort = p;
    emit changed();
}

void WhatsAppProxyConfigModel::setPublicHost(const QString &host)
{
    const QString h = host.trimmed();
    if (!isValidPublicHost(h) || m_protocolConfig.publicHost == h) {
        return;
    }
    m_protocolConfig.publicHost = h;
    emit changed();
}

void WhatsAppProxyConfigModel::setEnabled(bool enabled)
{
    if (m_protocolConfig.isEnabled == enabled) {
        return;
    }
    m_protocolConfig.isEnabled = enabled;
    emit changed();
}

QString WhatsAppProxyConfigModel::getPort() const
{
    return m_protocolConfig.effectivePort();
}

QString WhatsAppProxyConfigModel::getMediaPort() const
{
    return m_protocolConfig.effectiveMediaPort();
}

QString WhatsAppProxyConfigModel::getPublicHost() const
{
    return m_protocolConfig.publicHost;
}

QString WhatsAppProxyConfigModel::defaultPort() const
{
    return QString(protocols::whatsAppProxy::defaultPort);
}

QString WhatsAppProxyConfigModel::defaultMediaPort() const
{
    return QString(protocols::whatsAppProxy::defaultMediaPort);
}

bool WhatsAppProxyConfigModel::isValidPort(const QString &port) const
{
    bool ok = false;
    const int n = port.toInt(&ok);
    return ok && n >= 1 && n <= 65535;
}

bool WhatsAppProxyConfigModel::isValidPublicHost(const QString &host) const
{
    const QString t = host.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (t.length() > 253) {
        return false;
    }
    const QHostAddress address(t);
    if (address.protocol() == QHostAddress::IPv4Protocol) {
        return NetworkUtilities::checkIPv4Format(t);
    }
    if (address.protocol() == QHostAddress::IPv6Protocol) {
        return !address.isNull() && !address.isLoopback();
    }
    return kHostnameRe.match(t).hasMatch();
}

QString WhatsAppProxyConfigModel::sanitizePortFieldText(const QString &input) const
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

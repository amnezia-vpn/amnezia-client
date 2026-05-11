// SPDX-License-Identifier: GPL-3.0-or-later

#include "master_dns_vpn_service.h"

#include "engine.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

MasterDnsVpnService &MasterDnsVpnService::getInstance()
{
    static MasterDnsVpnService instance;
    return instance;
}

MasterDnsVpnService::MasterDnsVpnService() = default;
MasterDnsVpnService::~MasterDnsVpnService() = default;

bool MasterDnsVpnService::start(const QString &configJson)
{
    qDebug() << "MasterDnsVpnService::start";

    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "MasterDnsVpnService::start: invalid JSON:" << err.errorString();
        return false;
    }

    if (m_engine) {
        // Tear down the previous instance — caller (GUI) is allowed to
        // start over without an explicit stop in between.
        m_engine->stop();
        m_engine.reset();
    }

    m_engine = std::make_unique<amnezia::masterdnsvpn::Engine>();
    if (!m_engine->start(doc.object())) {
        qWarning() << "MasterDnsVpnService::start: engine start failed:"
                   << m_engine->lastError();
        m_engine.reset();
        return false;
    }
    return true;
}

bool MasterDnsVpnService::stop()
{
    qDebug() << "MasterDnsVpnService::stop";
    if (!m_engine) {
        return true;
    }
    m_engine->stop();
    m_engine.reset();
    return true;
}

quint16 MasterDnsVpnService::socksPort() const
{
    return m_engine ? m_engine->socksPort() : 0;
}

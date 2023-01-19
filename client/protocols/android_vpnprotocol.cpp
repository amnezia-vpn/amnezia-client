#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTextCodec>
#include <QTimer>

#include "android_vpnprotocol.h"

#include "platforms/android/android_controller.h"


AndroidVpnProtocol::AndroidVpnProtocol(Proto protocol, const QJsonObject &configuration, QObject* parent)
    : VpnProtocol(configuration, parent),
      m_protocol(protocol)
{ }

ErrorCode AndroidVpnProtocol::start()
{
    AndroidController::instance()->setVpnConfig(m_rawConfig);
    return AndroidController::instance()->start();
}

void AndroidVpnProtocol::stop()
{
    qDebug() << "AndroidVpnProtocol::stop()";
    AndroidController::instance()->stop();
}

void AndroidVpnProtocol::connectionDataUpdated(QString totalRx, QString totalTx, QString endpoint, QString deviceIPv4)
{
    quint64 rxBytes = totalRx.toLongLong();
    quint64 txBytes = totalTx.toLongLong();

    setBytesChanged(rxBytes, txBytes);
}


#include "android_vpnprotocol.h"

#include "platforms/android/android_controller.h"


AndroidVpnProtocol::AndroidVpnProtocol(Proto protocol, const QJsonObject &configuration, QObject* parent)
    : VpnProtocol(configuration, parent)
{ }

ErrorCode AndroidVpnProtocol::start()
{
    qDebug() << "AndroidVpnProtocol::start()";
    return AndroidController::instance()->start(m_rawConfig);
}

void AndroidVpnProtocol::stop()
{
    qDebug() << "AndroidVpnProtocol::stop()";
    AndroidController::instance()->stop();
}

void AndroidVpnProtocol::connectionDataUpdated(quint64 rxBytes, quint64 txBytes)
{
    setBytesChanged(rxBytes, txBytes);
}


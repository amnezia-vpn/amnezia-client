//#include <QAndroidBinder>
//#include <QAndroidIntent>
//#include <QAndroidJniEnvironment>
//#include <QAndroidJniObject>
//#include <QAndroidParcel>
//#include <QAndroidServiceConnection>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTextCodec>
#include <QTimer>
//#include <QtAndroid>
#include <QtCore/private/qandroidextras_p.h>


#include "android_vpnprotocol.h"
#include "core/errorstrings.h"

#include "platforms/android/android_controller.h"


AndroidVpnProtocol::AndroidVpnProtocol(Proto protocol, const QJsonObject &configuration, QObject* parent)
    : VpnProtocol(configuration, parent),
      m_protocol(protocol)
{

}

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


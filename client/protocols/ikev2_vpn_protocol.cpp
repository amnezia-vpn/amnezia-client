#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QThread>

#include "debug.h"
#include "ikev2_vpn_protocol.h"
#include "utils.h"

Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    //m_configFile.setFileTemplate(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readIkev2Configuration(configuration);
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
    Ikev2Protocol::stop();
    QThread::msleep(200);
}

void Ikev2Protocol::stop()
{
#ifndef Q_OS_IOS

#endif
}

void Ikev2Protocol::readIkev2Configuration(const QJsonObject &configuration)
{
    m_config = configuration.value(config::key_ikev2_config_data).toObject();
}



ErrorCode Ikev2Protocol::start()
{
#ifndef Q_OS_IOS

    QByteArray cert = QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8());
    qDebug() << "Ikev2Protocol::start()" << cert;

    QTemporaryFile certFile;
    certFile.open();
    certFile.write(cert);
    certFile.close();


    return ErrorCode::NoError;

#endif
}


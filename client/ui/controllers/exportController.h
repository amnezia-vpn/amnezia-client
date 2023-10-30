#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>

#include "configurators/vpn_configurator.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/authResultReceiver.h"
#endif

class ExportController : public QObject
{
    Q_OBJECT
public:
    explicit ExportController(const QSharedPointer<ServersModel> &serversModel,
                              const QSharedPointer<ContainersModel> &containersModel,
                              const std::shared_ptr<Settings> &settings,
                              const std::shared_ptr<VpnConfigurator> &configurator, QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY exportConfigChanged)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY exportConfigChanged)
    Q_PROPERTY(QString config READ getConfig NOTIFY exportConfigChanged)

public slots:
    void generateFullAccessConfig();
#if defined(Q_OS_ANDROID)
    void generateFullAccessConfigAndroid();
#endif
    void generateConnectionConfig();
    void generateOpenVpnConfig();
    void generateWireGuardConfig();
    void generateShadowSocksConfig();
    void generateCloakConfig();

    QString getConfig();
    QList<QString> getQrCodes();

    void exportConfig(const QString &fileName);

signals:
    void generateConfig(int type);
    void exportErrorOccurred(const QString &errorMessage);

    void exportConfigChanged();

    void saveFile(const QString &fileName, const QString &data);

private:
    QList<QString> generateQrCodeImageSeries(const QByteArray &data);
    QString svgToBase64(const QString &image);

    int getQrCodesCount();

    void clearPreviousConfig();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    QString m_config;
    QList<QString> m_qrCodes;

#ifdef Q_OS_ANDROID
    QSharedPointer<AuthResultNotifier> m_authResultNotifier;
    QSharedPointer<QAndroidActivityResultReceiver> m_authResultReceiver;
#endif
};

#endif // EXPORTCONTROLLER_H

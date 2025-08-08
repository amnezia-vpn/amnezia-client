#ifndef APICONFIGUICONTROLLER_H
#define APICONFIGUICONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include "core/defs.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/servers_model.h"

class Settings;
class ApiConfigsController;

class ApiConfigUIController : public QObject
{
    Q_OBJECT
public:
    explicit ApiConfigUIController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                   const QSharedPointer<ApiConfigsController> &coreController,
                                   const std::shared_ptr<Settings> &settings,
                                   QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY vpnKeyExportReady)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY vpnKeyExportReady)
    Q_PROPERTY(QString vpnKey READ getVpnKey NOTIFY vpnKeyExportReady)

public slots:
    bool exportNativeConfig(const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(const QString &serverCountryCode);
    void prepareVpnKeyExport();
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importServiceFromGateway();
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice();
    bool deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode);

    bool isConfigValid();

    void setCurrentProtocol(const QString &protocolName);
    bool isVlessProtocol();

signals:
    void errorOccurred(amnezia::ErrorCode errorCode);
    void installServerFromApiFinished(const QString &message);
    void changeApiCountryFinished(const QString &message);
    void reloadServerFromApiFinished(const QString &message);
    void updateServerFromApiFinished();
    void vpnKeyExportReady();

private:
    QList<QString> getQrCodes();
    int getQrCodesCount();
    QString getVpnKey();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    QSharedPointer<ApiConfigsController> m_coreController;
    std::shared_ptr<Settings> m_settings;
};

#endif // APICONFIGUICONTROLLER_H



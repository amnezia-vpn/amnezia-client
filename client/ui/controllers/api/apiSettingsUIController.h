#ifndef APISETTINGSUICONTROLLER_H
#define APISETTINGSUICONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include "core/defs.h"
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"
#include "ui/models/servers_model.h"

class Settings;
class ApiSettingsController;

class ApiSettingsUIController : public QObject
{
    Q_OBJECT
public:
    explicit ApiSettingsUIController(const QSharedPointer<ServersModel> &serversModel,
                                     const QSharedPointer<ApiAccountInfoModel> &apiAccountInfoModel,
                                     const QSharedPointer<ApiCountryModel> &apiCountryModel,
                                     const QSharedPointer<ApiDevicesModel> &apiDevicesModel,
                                     const QSharedPointer<ApiSettingsController> &coreController,
                                     const std::shared_ptr<Settings> &settings,
                                     QObject *parent = nullptr);
    ~ApiSettingsUIController();

public slots:
    bool getAccountInfo(bool reload);
    void updateApiCountryModel();
    void updateApiDevicesModel();

signals:
    void errorOccurred(amnezia::ErrorCode errorCode);

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiAccountInfoModel> m_apiAccountInfoModel;
    QSharedPointer<ApiCountryModel> m_apiCountryModel;
    QSharedPointer<ApiDevicesModel> m_apiDevicesModel;
    QSharedPointer<ApiSettingsController> m_coreController;
    std::shared_ptr<Settings> m_settings;
};

#endif // APISETTINGSUICONTROLLER_H



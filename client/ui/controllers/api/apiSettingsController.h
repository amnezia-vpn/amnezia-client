#ifndef APISETTINGSCONTROLLER_H
#define APISETTINGSCONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"
#include "ui/models/servers_model.h"

class ApiSettingsController : public QObject
{
    Q_OBJECT
public:
    ApiSettingsController(ServersController* serversController,
                          ServersModel* serversModel,
                          ApiAccountInfoModel* apiAccountInfoModel,
                          ApiCountryModel* apiCountryModel,
                          ApiDevicesModel* apiDevicesModel,
                          QAppSettingsRepository* appSettingsRepository,
                          QObject *parent = nullptr);
    ~ApiSettingsController();

public slots:
    bool getAccountInfo(bool reload);
    void updateApiCountryModel();
    void updateApiDevicesModel();

signals:
    void errorOccurred(ErrorCode errorCode);

private:
    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ApiAccountInfoModel* m_apiAccountInfoModel;
    ApiCountryModel* m_apiCountryModel;
    ApiDevicesModel* m_apiDevicesModel;
    QAppSettingsRepository* m_appSettingsRepository;
};

#endif // APISETTINGSCONTROLLER_H

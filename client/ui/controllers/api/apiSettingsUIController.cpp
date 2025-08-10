#include "apiSettingsUIController.h"

#include "core/controllers/api/apiSettingsController.h"

ApiSettingsUIController::ApiSettingsUIController(const QSharedPointer<ServersModel> &serversModel,
                                                 const QSharedPointer<ApiAccountInfoModel> &apiAccountInfoModel,
                                                 const QSharedPointer<ApiCountryModel> &apiCountryModel,
                                                 const QSharedPointer<ApiDevicesModel> &apiDevicesModel,
                                                  const QSharedPointer<ApiSettingsController> &coreController,
                                                 QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
       m_coreController(coreController)
{
    
}

ApiSettingsUIController::~ApiSettingsUIController()
{
}

bool ApiSettingsUIController::getAccountInfo(bool reload)
{
    auto errorCode = m_coreController->getAccountInfo(reload);
    if (errorCode != amnezia::ErrorCode::NoError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError;
}

void ApiSettingsUIController::updateApiCountryModel()
{
    m_coreController->updateApiCountryModel();
}

void ApiSettingsUIController::updateApiDevicesModel()
{
    m_coreController->updateApiDevicesModel();
}



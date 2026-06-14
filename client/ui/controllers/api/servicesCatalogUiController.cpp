#include "servicesCatalogUiController.h"

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

ServicesCatalogUiController::ServicesCatalogUiController(ServicesCatalogController* servicesCatalogController,
                                                         ApiServicesModel* apiServicesModel,
                                                         QObject *parent)
    : QObject(parent),
      m_servicesCatalogController(servicesCatalogController),
      m_apiServicesModel(apiServicesModel)
{
}

bool ServicesCatalogUiController::fillAvailableServices()
{
    QJsonObject servicesData;
    ErrorCode errorCode = m_servicesCatalogController->fillAvailableServices(servicesData);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    m_apiServicesModel->updateModel(servicesData);
    if (m_apiServicesModel->rowCount() > 0) {
        m_apiServicesModel->setServiceIndex(0);
    }
    return true;
}

QJsonObject ServicesCatalogUiController::getSelectedServiceInfo()
{
    return m_apiServicesModel->getSelectedServiceInfo();
}

QString ServicesCatalogUiController::getSelectedServiceType()
{
    return m_apiServicesModel->getSelectedServiceType();
}

QString ServicesCatalogUiController::getSelectedServiceProtocol()
{
    return m_apiServicesModel->getSelectedServiceProtocol();
}

QString ServicesCatalogUiController::getSelectedServiceName()
{
    return m_apiServicesModel->getSelectedServiceName();
}

QJsonArray ServicesCatalogUiController::getSelectedServiceCountries()
{
    return m_apiServicesModel->getSelectedServiceCountries();
}

QString ServicesCatalogUiController::getCountryCode()
{
    return m_apiServicesModel->getCountryCode();
}

QString ServicesCatalogUiController::getStoreEndpoint()
{
    return m_apiServicesModel->getStoreEndpoint();
}

QVariant ServicesCatalogUiController::getSelectedServiceData(const QString &roleString)
{
    return m_apiServicesModel->getSelectedServiceData(roleString);
}


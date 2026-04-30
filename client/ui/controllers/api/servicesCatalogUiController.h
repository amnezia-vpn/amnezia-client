#ifndef SERVICESCATALOGUICONTROLLER_H
#define SERVICESCATALOGUICONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "ui/models/api/apiServicesModel.h"

class ServicesCatalogUiController : public QObject
{
    Q_OBJECT

public:
    explicit ServicesCatalogUiController(ServicesCatalogController* servicesCatalogController,
                                         ApiServicesModel* apiServicesModel,
                                         QObject *parent = nullptr);

public slots:
    bool fillAvailableServices();

    QJsonObject getSelectedServiceInfo();
    QString getSelectedServiceType();
    QString getSelectedServiceProtocol();
    QString getSelectedServiceName();
    QJsonArray getSelectedServiceCountries();
    QString getCountryCode();
    QString getStoreEndpoint();
    QVariant getSelectedServiceData(const QString &roleString);

signals:
    void errorOccurred(ErrorCode errorCode);

private:
    ServicesCatalogController* m_servicesCatalogController;
    ApiServicesModel* m_apiServicesModel;
};

#endif // SERVICESCATALOGUICONTROLLER_H


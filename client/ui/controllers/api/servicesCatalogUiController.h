#ifndef SERVICESCATALOGUICONTROLLER_H
#define SERVICESCATALOGUICONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

#include "core/defs.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "ui/models/api/apiServicesModel.h"

class ServicesCatalogUiController : public QObject
{
    Q_OBJECT

public:
    explicit ServicesCatalogUiController(const QSharedPointer<ServicesCatalogController> &servicesCatalogController,
                                         const QSharedPointer<ApiServicesModel> &apiServicesModel,
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
    QSharedPointer<ServicesCatalogController> m_servicesCatalogController;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
};

#endif // SERVICESCATALOGUICONTROLLER_H


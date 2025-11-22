#ifndef ALLOWEDDNSUICONTROLLER_H
#define ALLOWEDDNSUICONTROLLER_H

#include <QObject>

#include "core/controllers/allowedDnsController.h"
#include "ui/models/allowed_dns_model.h"

class AllowedDnsUiController : public QObject
{
    Q_OBJECT
public:
    explicit AllowedDnsUiController(const QSharedPointer<AllowedDnsController> &allowedDnsController,
                                     const QSharedPointer<AllowedDnsModel> &allowedDnsModel,
                                     QObject *parent = nullptr);

public slots:
    void addDns(QString ip);
    void removeDns(int index);

    void importDns(const QString &fileName, bool replaceExisting);
    void exportDns(const QString &fileName);

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

    void saveFile(const QString &fileName, const QString &data);

private:
    QSharedPointer<AllowedDnsController> m_allowedDnsController;
    QSharedPointer<AllowedDnsModel> m_allowedDnsModel;
};

#endif // ALLOWEDDNSUICONTROLLER_H 

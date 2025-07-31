#ifndef ALLOWEDDNSUICONTROLLER_H
#define ALLOWEDDNSUICONTROLLER_H

#include <QObject>

#include "ui/models/allowed_dns_model.h"
#include "core/controllers/dnsController.h"

class AllowedDnsUIController : public QObject
{
    Q_OBJECT
public:
    explicit AllowedDnsUIController(QSharedPointer<DnsController> dnsController,
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
    QSharedPointer<DnsController> m_dnsController;
    QSharedPointer<AllowedDnsModel> m_allowedDnsModel;
};

#endif // ALLOWEDDNSUICONTROLLER_H 

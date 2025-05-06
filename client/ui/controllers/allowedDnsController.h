#ifndef ALLOWEDDNSCONTROLLER_H
#define ALLOWEDDNSCONTROLLER_H

#include <QObject>

#include "settings.h"
#include "ui/models/allowed_dns_model.h"

class AllowedDnsController : public QObject
{
    Q_OBJECT
public:
    explicit AllowedDnsController(const std::shared_ptr<Settings> &settings,
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
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<AllowedDnsModel> m_allowedDnsModel;
};

#endif // ALLOWEDDNSCONTROLLER_H 

#ifndef ALLOWEDDNSUICONTROLLER_H
#define ALLOWEDDNSUICONTROLLER_H

#include <QObject>

#include "core/controllers/allowedDnsController.h"
#include "ui/models/allowedDnsModel.h"

class AllowedDnsUiController : public QObject
{
    Q_OBJECT
public:
    explicit AllowedDnsUiController(AllowedDnsController* allowedDnsController,
                                     AllowedDnsModel* allowedDnsModel,
                                     QObject *parent = nullptr);

public slots:
    void addDns(QString ip);
    void removeDns(int index);

    void importDns(const QString &fileName, bool replaceExisting);
    void exportDns(const QString &fileName);

    void updateModel();

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

    void saveFile(const QString &fileName, const QString &data);

private:
    AllowedDnsController* m_allowedDnsController;
    AllowedDnsModel* m_allowedDnsModel;
};

#endif // ALLOWEDDNSUICONTROLLER_H 

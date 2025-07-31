#ifndef SITESPLITUICONTROLLER_H
#define SITESPLITUICONTROLLER_H

#include <QObject>

#include "ui/models/sites_model.h"
#include "core/controllers/splitTunnelingController.h"

class SiteSplitUIController : public QObject
{
    Q_OBJECT
public:
    explicit SiteSplitUIController(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                                   const QSharedPointer<SitesModel> &sitesModel, 
                                   QObject *parent = nullptr);

public slots:
    void addSite(QString hostname);
    void removeSite(int index);

    void importSites(const QString &fileName, bool replaceExisting);
    void exportSites(const QString &fileName);

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

    void saveFile(const QString &fileName, const QString &data);

private:
    QSharedPointer<SplitTunnelingController> m_splitTunnelingController;
    QSharedPointer<SitesModel> m_sitesModel;
};

#endif // SITESPLITUICONTROLLER_H

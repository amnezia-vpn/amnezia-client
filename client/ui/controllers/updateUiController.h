#ifndef UPDATEUICONTROLLER_H
#define UPDATEUICONTROLLER_H

#include <QObject>

#include "core/controllers/updateController.h"

class MarketplaceUpdateController;

class UpdateUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString changelogText READ getChangelogText NOTIFY updateFound)
    Q_PROPERTY(QString headerText READ getHeaderText NOTIFY updateFound)

public:
    explicit UpdateUiController(UpdateController* updateController,
                                MarketplaceUpdateController* marketplaceUpdateController = nullptr,
                                QObject *parent = nullptr);

    QString getHeaderText() const;
    QString getChangelogText() const;
    QString getVersion() const;

public slots:
    void checkForUpdates();
    void runInstaller();

signals:
    void updateFound();

private:
    UpdateController* m_updateController;
    MarketplaceUpdateController* m_marketplaceUpdateController;
};

#endif // UPDATEUICONTROLLER_H

#ifndef SITES_LOGIC_H
#define SITES_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;
class SitesModel;

class SitesLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, labelSitesAddCustomText)
    AUTO_PROPERTY(QObject*, tableViewSitesModel)
    AUTO_PROPERTY(QString, lineEditSitesAddCustomText)

public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonAddCustomSitesClicked();
    Q_INVOKABLE void onPushButtonSitesDeleteClicked(QStringList items);
    Q_INVOKABLE void onPushButtonSitesImportClicked(const QString &fileName);
    Q_INVOKABLE void onPushButtonSitesExportClicked();


public:
    explicit SitesLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~SitesLogic() = default;

    QMap<Settings::RouteMode, SitesModel *> sitesModels;
};
#endif // SITES_LOGIC_H

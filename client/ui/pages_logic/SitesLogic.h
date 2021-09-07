#ifndef SITES_LOGIC_H
#define SITES_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;
class SitesModel;

class SitesLogic : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateSitesPage();

    Q_PROPERTY(QString labelSitesAddCustomText READ getLabelSitesAddCustomText WRITE setLabelSitesAddCustomText NOTIFY labelSitesAddCustomTextChanged)
    Q_PROPERTY(QObject* tableViewSitesModel READ getTableViewSitesModel NOTIFY tableViewSitesModelChanged)
    Q_PROPERTY(QString lineEditSitesAddCustomText READ getLineEditSitesAddCustomText WRITE setLineEditSitesAddCustomText NOTIFY lineEditSitesAddCustomTextChanged)

    Q_INVOKABLE void onPushButtonAddCustomSitesClicked();
    Q_INVOKABLE void onPushButtonSitesDeleteClicked(int row);
    Q_INVOKABLE void onPushButtonSitesImportClicked(const QString &fileName);

public:
    explicit SitesLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~SitesLogic() = default;



    QString getLabelSitesAddCustomText() const;
    void setLabelSitesAddCustomText(const QString &labelSitesAddCustomText);
    QObject* getTableViewSitesModel() const;
    void setTableViewSitesModel(QObject *tableViewSitesModel);
    QString getLineEditSitesAddCustomText() const;
    void setLineEditSitesAddCustomText(const QString &lineEditSitesAddCustomText);

signals:
    void labelSitesAddCustomTextChanged();
    void tableViewSitesModelChanged();
    void lineEditSitesAddCustomTextChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;
    UiLogic *uiLogic() const { return m_uiLogic; }

    QString m_labelSitesAddCustomText;
    QObject* m_tableViewSitesModel;
    QString m_lineEditSitesAddCustomText;

    QMap<Settings::RouteMode, SitesModel *> sitesModels;
};
#endif // SITES_LOGIC_H

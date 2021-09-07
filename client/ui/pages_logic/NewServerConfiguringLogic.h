#ifndef NEW_SERVER_CONFIGURING_LOGIC_H
#define NEW_SERVER_CONFIGURING_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class NewServerConfiguringLogic : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool pageNewServerConfiguringEnabled READ getPageNewServerConfiguringEnabled WRITE setPageNewServerConfiguringEnabled NOTIFY pageNewServerConfiguringEnabledChanged)
    Q_PROPERTY(bool labelNewServerConfiguringWaitInfoVisible READ getLabelNewServerConfiguringWaitInfoVisible WRITE setLabelNewServerConfiguringWaitInfoVisible NOTIFY labelNewServerConfiguringWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelNewServerConfiguringWaitInfoText READ getLabelNewServerConfiguringWaitInfoText WRITE setLabelNewServerConfiguringWaitInfoText NOTIFY labelNewServerConfiguringWaitInfoTextChanged)
    Q_PROPERTY(bool progressBarNewServerConfiguringVisible READ getProgressBarNewServerConfiguringVisible WRITE setProgressBarNewServerConfiguringVisible NOTIFY progressBarNewServerConfiguringVisibleChanged)
    Q_PROPERTY(int progressBarNewServerConfiguringMaximium READ getProgressBarNewServerConfiguringMaximium WRITE setProgressBarNewServerConfiguringMaximium NOTIFY progressBarNewServerConfiguringMaximiumChanged)
    Q_PROPERTY(bool progressBarNewServerConfiguringTextVisible READ getProgressBarNewServerConfiguringTextVisible WRITE setProgressBarNewServerConfiguringTextVisible NOTIFY progressBarNewServerConfiguringTextVisibleChanged)
    Q_PROPERTY(QString progressBarNewServerConfiguringText READ getProgressBarNewServerConfiguringText WRITE setProgressBarNewServerConfiguringText NOTIFY progressBarNewServerConfiguringTextChanged)

public:
    explicit NewServerConfiguringLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerConfiguringLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // NEW_SERVER_CONFIGURING_LOGIC_H

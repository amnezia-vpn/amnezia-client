#ifndef NEW_SERVER_CONFIGURING_LOGIC_H
#define NEW_SERVER_CONFIGURING_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class NewServerConfiguringLogic : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(double progressBarNewServerConfiguringValue READ getProgressBarNewServerConfiguringValue WRITE setProgressBarNewServerConfiguringValue NOTIFY progressBarNewServerConfiguringValueChanged)
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

    double getProgressBarNewServerConfiguringValue() const;
    void setProgressBarNewServerConfiguringValue(double progressBarNewServerConfiguringValue);

    bool getPageNewServerConfiguringEnabled() const;
    void setPageNewServerConfiguringEnabled(bool pageNewServerConfiguringEnabled);
    bool getLabelNewServerConfiguringWaitInfoVisible() const;
    void setLabelNewServerConfiguringWaitInfoVisible(bool labelNewServerConfiguringWaitInfoVisible);
    QString getLabelNewServerConfiguringWaitInfoText() const;
    void setLabelNewServerConfiguringWaitInfoText(const QString &labelNewServerConfiguringWaitInfoText);
    bool getProgressBarNewServerConfiguringVisible() const;
    void setProgressBarNewServerConfiguringVisible(bool progressBarNewServerConfiguringVisible);
    int getProgressBarNewServerConfiguringMaximium() const;
    void setProgressBarNewServerConfiguringMaximium(int progressBarNewServerConfiguringMaximium);
    bool getProgressBarNewServerConfiguringTextVisible() const;
    void setProgressBarNewServerConfiguringTextVisible(bool progressBarNewServerConfiguringTextVisible);
    QString getProgressBarNewServerConfiguringText() const;
    void setProgressBarNewServerConfiguringText(const QString &progressBarNewServerConfiguringText);

signals:
    void progressBarNewServerConfiguringValueChanged();
    void pageNewServerConfiguringEnabledChanged();
    void labelNewServerConfiguringWaitInfoVisibleChanged();
    void labelNewServerConfiguringWaitInfoTextChanged();
    void progressBarNewServerConfiguringVisibleChanged();
    void progressBarNewServerConfiguringMaximiumChanged();
    void progressBarNewServerConfiguringTextVisibleChanged();
    void progressBarNewServerConfiguringTextChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;
    UiLogic *uiLogic() const { return m_uiLogic; }

    double m_progressBarNewServerConfiguringValue;
    bool m_pageNewServerConfiguringEnabled;
    bool m_labelNewServerConfiguringWaitInfoVisible;
    QString m_labelNewServerConfiguringWaitInfoText;
    bool m_progressBarNewServerConfiguringVisible;
    int m_progressBarNewServerConfiguringMaximium;
    bool m_progressBarNewServerConfiguringTextVisible;
    QString m_progressBarNewServerConfiguringText;

};
#endif // NEW_SERVER_CONFIGURING_LOGIC_H

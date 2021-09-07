#ifndef SERVER_CONTAINERS_LOGIC_H
#define SERVER_CONTAINERS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerContainersLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateServerContainersPage();

    Q_PROPERTY(bool pageServerContainersEnabled READ getPageServerContainersEnabled WRITE setPageServerContainersEnabled NOTIFY pageServerContainersEnabledChanged)
    Q_PROPERTY(int progressBarProtocolsContainerReinstallValue READ getProgressBarProtocolsContainerReinstallValue WRITE setProgressBarProtocolsContainerReinstallValue NOTIFY progressBarProtocolsContainerReinstallValueChanged)
    Q_PROPERTY(int progressBarProtocolsContainerReinstallMaximium READ getProgressBarProtocolsContainerReinstallMaximium WRITE setProgressBarProtocolsContainerReinstallMaximium NOTIFY progressBarProtocolsContainerReinstallMaximiumChanged)

    Q_PROPERTY(bool pushButtonProtoOpenvpnContInstallChecked READ getPushButtonProtoOpenvpnContInstallChecked WRITE setPushButtonProtoOpenvpnContInstallChecked NOTIFY pushButtonProtoOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContInstallChecked READ getPushButtonProtoSsOpenvpnContInstallChecked WRITE setPushButtonProtoSsOpenvpnContInstallChecked NOTIFY pushButtonProtoSsOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContInstallChecked READ getPushButtonProtoCloakOpenvpnContInstallChecked WRITE setPushButtonProtoCloakOpenvpnContInstallChecked NOTIFY pushButtonProtoCloakOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContInstallChecked READ getPushButtonProtoWireguardContInstallChecked WRITE setPushButtonProtoWireguardContInstallChecked NOTIFY pushButtonProtoWireguardContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContInstallEnabled READ getPushButtonProtoOpenvpnContInstallEnabled WRITE setPushButtonProtoOpenvpnContInstallEnabled NOTIFY pushButtonProtoOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContInstallEnabled READ getPushButtonProtoSsOpenvpnContInstallEnabled WRITE setPushButtonProtoSsOpenvpnContInstallEnabled NOTIFY pushButtonProtoSsOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContInstallEnabled READ getPushButtonProtoCloakOpenvpnContInstallEnabled WRITE setPushButtonProtoCloakOpenvpnContInstallEnabled NOTIFY pushButtonProtoCloakOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContInstallEnabled READ getPushButtonProtoWireguardContInstallEnabled WRITE setPushButtonProtoWireguardContInstallEnabled NOTIFY pushButtonProtoWireguardContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContDefaultChecked READ getPushButtonProtoOpenvpnContDefaultChecked WRITE setPushButtonProtoOpenvpnContDefaultChecked NOTIFY pushButtonProtoOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContDefaultChecked READ getPushButtonProtoSsOpenvpnContDefaultChecked WRITE setPushButtonProtoSsOpenvpnContDefaultChecked NOTIFY pushButtonProtoSsOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContDefaultChecked READ getPushButtonProtoCloakOpenvpnContDefaultChecked WRITE setPushButtonProtoCloakOpenvpnContDefaultChecked NOTIFY pushButtonProtoCloakOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContDefaultChecked READ getPushButtonProtoWireguardContDefaultChecked WRITE setPushButtonProtoWireguardContDefaultChecked NOTIFY pushButtonProtoWireguardContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContDefaultVisible READ getPushButtonProtoOpenvpnContDefaultVisible WRITE setPushButtonProtoOpenvpnContDefaultVisible NOTIFY pushButtonProtoOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContDefaultVisible READ getPushButtonProtoSsOpenvpnContDefaultVisible WRITE setPushButtonProtoSsOpenvpnContDefaultVisible NOTIFY pushButtonProtoSsOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContDefaultVisible READ getPushButtonProtoCloakOpenvpnContDefaultVisible WRITE setPushButtonProtoCloakOpenvpnContDefaultVisible NOTIFY pushButtonProtoCloakOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContDefaultVisible READ getPushButtonProtoWireguardContDefaultVisible WRITE setPushButtonProtoWireguardContDefaultVisible NOTIFY pushButtonProtoWireguardContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContShareVisible READ getPushButtonProtoOpenvpnContShareVisible WRITE setPushButtonProtoOpenvpnContShareVisible NOTIFY pushButtonProtoOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContShareVisible READ getPushButtonProtoSsOpenvpnContShareVisible WRITE setPushButtonProtoSsOpenvpnContShareVisible NOTIFY pushButtonProtoSsOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContShareVisible READ getPushButtonProtoCloakOpenvpnContShareVisible WRITE setPushButtonProtoCloakOpenvpnContShareVisible NOTIFY pushButtonProtoCloakOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContShareVisible READ getPushButtonProtoWireguardContShareVisible WRITE setPushButtonProtoWireguardContShareVisible NOTIFY pushButtonProtoWireguardContShareVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSettingsVisible READ getFrameOpenvpnSettingsVisible WRITE setFrameOpenvpnSettingsVisible NOTIFY frameOpenvpnSettingsVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSsSettingsVisible READ getFrameOpenvpnSsSettingsVisible WRITE setFrameOpenvpnSsSettingsVisible NOTIFY frameOpenvpnSsSettingsVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSsCloakSettingsVisible READ getFrameOpenvpnSsCloakSettingsVisible WRITE setFrameOpenvpnSsCloakSettingsVisible NOTIFY frameOpenvpnSsCloakSettingsVisibleChanged)
    Q_PROPERTY(bool progressBarProtocolsContainerReinstallVisible READ getProgressBarProtocolsContainerReinstallVisible WRITE setProgressBarProtocolsContainerReinstallVisible NOTIFY progressBarProtocolsContainerReinstallVisibleChanged)

    Q_PROPERTY(bool frameWireguardSettingsVisible READ getFrameWireguardSettingsVisible WRITE setFrameWireguardSettingsVisible NOTIFY frameWireguardSettingsVisibleChanged)
    Q_PROPERTY(bool frameWireguardVisible READ getFrameWireguardVisible WRITE setFrameWireguardVisible NOTIFY frameWireguardVisibleChanged)

    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContSsConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContCloakConfigClicked();

    Q_INVOKABLE void onPushButtonProtoOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContSsConfigClicked();

public:
    explicit ServerContainersLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerContainersLogic() = default;

    void setupProtocolsPageConnections();

    bool getPageServerContainersEnabled() const;
    void setPageServerContainersEnabled(bool pageServerContainersEnabled);
    int getProgressBarProtocolsContainerReinstallValue() const;
    void setProgressBarProtocolsContainerReinstallValue(int progressBarProtocolsContainerReinstallValue);
    int getProgressBarProtocolsContainerReinstallMaximium() const;
    void setProgressBarProtocolsContainerReinstallMaximium(int progressBarProtocolsContainerReinstallMaximium);

    bool getPushButtonProtoOpenvpnContInstallChecked() const;
    void setPushButtonProtoOpenvpnContInstallChecked(bool pushButtonProtoOpenvpnContInstallChecked);
    bool getPushButtonProtoSsOpenvpnContInstallChecked() const;
    void setPushButtonProtoSsOpenvpnContInstallChecked(bool pushButtonProtoSsOpenvpnContInstallChecked);
    bool getPushButtonProtoCloakOpenvpnContInstallChecked() const;
    void setPushButtonProtoCloakOpenvpnContInstallChecked(bool pushButtonProtoCloakOpenvpnContInstallChecked);
    bool getPushButtonProtoWireguardContInstallChecked() const;
    void setPushButtonProtoWireguardContInstallChecked(bool pushButtonProtoWireguardContInstallChecked);
    bool getPushButtonProtoOpenvpnContInstallEnabled() const;
    void setPushButtonProtoOpenvpnContInstallEnabled(bool pushButtonProtoOpenvpnContInstallEnabled);
    bool getPushButtonProtoSsOpenvpnContInstallEnabled() const;
    void setPushButtonProtoSsOpenvpnContInstallEnabled(bool pushButtonProtoSsOpenvpnContInstallEnabled);
    bool getPushButtonProtoCloakOpenvpnContInstallEnabled() const;
    void setPushButtonProtoCloakOpenvpnContInstallEnabled(bool pushButtonProtoCloakOpenvpnContInstallEnabled);
    bool getPushButtonProtoWireguardContInstallEnabled() const;
    void setPushButtonProtoWireguardContInstallEnabled(bool pushButtonProtoWireguardContInstallEnabled);
    bool getPushButtonProtoOpenvpnContDefaultChecked() const;
    void setPushButtonProtoOpenvpnContDefaultChecked(bool pushButtonProtoOpenvpnContDefaultChecked);
    bool getPushButtonProtoSsOpenvpnContDefaultChecked() const;
    void setPushButtonProtoSsOpenvpnContDefaultChecked(bool pushButtonProtoSsOpenvpnContDefaultChecked);
    bool getPushButtonProtoCloakOpenvpnContDefaultChecked() const;
    void setPushButtonProtoCloakOpenvpnContDefaultChecked(bool pushButtonProtoCloakOpenvpnContDefaultChecked);
    bool getPushButtonProtoWireguardContDefaultChecked() const;
    void setPushButtonProtoWireguardContDefaultChecked(bool pushButtonProtoWireguardContDefaultChecked);
    bool getPushButtonProtoOpenvpnContDefaultVisible() const;
    void setPushButtonProtoOpenvpnContDefaultVisible(bool pushButtonProtoOpenvpnContDefaultVisible);
    bool getPushButtonProtoSsOpenvpnContDefaultVisible() const;
    void setPushButtonProtoSsOpenvpnContDefaultVisible(bool pushButtonProtoSsOpenvpnContDefaultVisible);
    bool getPushButtonProtoCloakOpenvpnContDefaultVisible() const;
    void setPushButtonProtoCloakOpenvpnContDefaultVisible(bool pushButtonProtoCloakOpenvpnContDefaultVisible);
    bool getPushButtonProtoWireguardContDefaultVisible() const;
    void setPushButtonProtoWireguardContDefaultVisible(bool pushButtonProtoWireguardContDefaultVisible);
    bool getPushButtonProtoOpenvpnContShareVisible() const;
    void setPushButtonProtoOpenvpnContShareVisible(bool pushButtonProtoOpenvpnContShareVisible);
    bool getPushButtonProtoSsOpenvpnContShareVisible() const;
    void setPushButtonProtoSsOpenvpnContShareVisible(bool pushButtonProtoSsOpenvpnContShareVisible);
    bool getPushButtonProtoCloakOpenvpnContShareVisible() const;
    void setPushButtonProtoCloakOpenvpnContShareVisible(bool pushButtonProtoCloakOpenvpnContShareVisible);
    bool getPushButtonProtoWireguardContShareVisible() const;
    void setPushButtonProtoWireguardContShareVisible(bool pushButtonProtoWireguardContShareVisible);
    bool getFrameOpenvpnSettingsVisible() const;
    void setFrameOpenvpnSettingsVisible(bool frameOpenvpnSettingsVisible);
    bool getFrameOpenvpnSsSettingsVisible() const;
    void setFrameOpenvpnSsSettingsVisible(bool frameOpenvpnSsSettingsVisible);
    bool getFrameOpenvpnSsCloakSettingsVisible() const;
    void setFrameOpenvpnSsCloakSettingsVisible(bool frameOpenvpnSsCloakSettingsVisible);
    bool getProgressBarProtocolsContainerReinstallVisible() const;
    void setProgressBarProtocolsContainerReinstallVisible(bool progressBarProtocolsContainerReinstallVisible);

    bool getFrameWireguardSettingsVisible() const;
    void setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible);
    bool getFrameWireguardVisible() const;
    void setFrameWireguardVisible(bool frameWireguardVisible);

signals:
    void pageServerContainersEnabledChanged();
    void progressBarProtocolsContainerReinstallValueChanged();
    void progressBarProtocolsContainerReinstallMaximiumChanged();

    void pushButtonProtoOpenvpnContInstallCheckedChanged();
    void pushButtonProtoSsOpenvpnContInstallCheckedChanged();
    void pushButtonProtoCloakOpenvpnContInstallCheckedChanged();
    void pushButtonProtoWireguardContInstallCheckedChanged();
    void pushButtonProtoOpenvpnContInstallEnabledChanged();
    void pushButtonProtoSsOpenvpnContInstallEnabledChanged();
    void pushButtonProtoCloakOpenvpnContInstallEnabledChanged();
    void pushButtonProtoWireguardContInstallEnabledChanged();
    void pushButtonProtoOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoSsOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoCloakOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoWireguardContDefaultCheckedChanged();
    void pushButtonProtoOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoSsOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoCloakOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoWireguardContDefaultVisibleChanged();
    void pushButtonProtoOpenvpnContShareVisibleChanged();
    void pushButtonProtoSsOpenvpnContShareVisibleChanged();
    void pushButtonProtoCloakOpenvpnContShareVisibleChanged();
    void pushButtonProtoWireguardContShareVisibleChanged();
    void frameOpenvpnSettingsVisibleChanged();
    void frameOpenvpnSsSettingsVisibleChanged();
    void frameOpenvpnSsCloakSettingsVisibleChanged();
    void progressBarProtocolsContainerReinstallVisibleChanged();

    void frameWireguardSettingsVisibleChanged();
    void frameWireguardVisibleChanged();

    void pushButtonProtoOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoSsOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoWireguardContDefaultClicked(bool checked);
    void pushButtonProtoOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoSsOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoWireguardContInstallClicked(bool checked);
    void pushButtonProtoOpenvpnContShareClicked(bool checked);
    void pushButtonProtoSsOpenvpnContShareClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContShareClicked(bool checked);
    void pushButtonProtoWireguardContShareClicked(bool checked);

private:


private slots:


private:
    bool m_pageServerContainersEnabled;
    int m_progressBarProtocolsContainerReinstallValue;
    int m_progressBarProtocolsContainerReinstallMaximium;

    bool m_pushButtonProtoOpenvpnContInstallChecked;
    bool m_pushButtonProtoSsOpenvpnContInstallChecked;
    bool m_pushButtonProtoCloakOpenvpnContInstallChecked;
    bool m_pushButtonProtoWireguardContInstallChecked;
    bool m_pushButtonProtoOpenvpnContInstallEnabled;
    bool m_pushButtonProtoSsOpenvpnContInstallEnabled;
    bool m_pushButtonProtoCloakOpenvpnContInstallEnabled;
    bool m_pushButtonProtoWireguardContInstallEnabled;
    bool m_pushButtonProtoOpenvpnContDefaultChecked;
    bool m_pushButtonProtoSsOpenvpnContDefaultChecked;
    bool m_pushButtonProtoCloakOpenvpnContDefaultChecked;
    bool m_pushButtonProtoWireguardContDefaultChecked;
    bool m_pushButtonProtoOpenvpnContDefaultVisible;
    bool m_pushButtonProtoSsOpenvpnContDefaultVisible;
    bool m_pushButtonProtoCloakOpenvpnContDefaultVisible;
    bool m_pushButtonProtoWireguardContDefaultVisible;
    bool m_pushButtonProtoOpenvpnContShareVisible;
    bool m_pushButtonProtoSsOpenvpnContShareVisible;
    bool m_pushButtonProtoCloakOpenvpnContShareVisible;
    bool m_pushButtonProtoWireguardContShareVisible;
    bool m_frameOpenvpnSettingsVisible;
    bool m_frameOpenvpnSsSettingsVisible;
    bool m_frameOpenvpnSsCloakSettingsVisible;
    bool m_progressBarProtocolsContainerReinstallVisible;

    bool m_frameWireguardSettingsVisible;
    bool m_frameWireguardVisible;
};
#endif // SERVER_CONTAINERS_LOGIC_H

#ifndef SERVER_SETTINGS_LOGIC_H
#define SERVER_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerSettingsLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateServerSettingsPage();

    Q_PROPERTY(bool pageServerSettingsEnabled READ getPageServerSettingsEnabled WRITE setPageServerSettingsEnabled NOTIFY pageServerSettingsEnabledChanged)

    Q_PROPERTY(bool labelServerSettingsWaitInfoVisible READ getLabelServerSettingsWaitInfoVisible WRITE setLabelServerSettingsWaitInfoVisible NOTIFY labelServerSettingsWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelServerSettingsWaitInfoText READ getLabelServerSettingsWaitInfoText WRITE setLabelServerSettingsWaitInfoText NOTIFY labelServerSettingsWaitInfoTextChanged)
    Q_PROPERTY(QString pushButtonServerSettingsClearText READ getPushButtonServerSettingsClearText WRITE setPushButtonServerSettingsClearText NOTIFY pushButtonServerSettingsClearTextChanged)
    Q_PROPERTY(QString pushButtonServerSettingsClearClientCacheText READ getPushButtonServerSettingsClearClientCacheText WRITE setPushButtonServerSettingsClearClientCacheText NOTIFY pushButtonServerSettingsClearClientCacheTextChanged)

    Q_PROPERTY(bool pushButtonServerSettingsClearVisible READ getPushButtonServerSettingsClearVisible WRITE setPushButtonServerSettingsClearVisible NOTIFY pushButtonServerSettingsClearVisibleChanged)
    Q_PROPERTY(bool pushButtonServerSettingsClearClientCacheVisible READ getPushButtonServerSettingsClearClientCacheVisible WRITE setPushButtonServerSettingsClearClientCacheVisible NOTIFY pushButtonServerSettingsClearClientCacheVisibleChanged)
    Q_PROPERTY(bool pushButtonServerSettingsShareFullVisible READ getPushButtonServerSettingsShareFullVisible WRITE setPushButtonServerSettingsShareFullVisible NOTIFY pushButtonServerSettingsShareFullVisibleChanged)
    Q_PROPERTY(QString labelServerSettingsServerText READ getLabelServerSettingsServerText WRITE setLabelServerSettingsServerText NOTIFY labelServerSettingsServerTextChanged)
    Q_PROPERTY(QString lineEditServerSettingsDescriptionText READ getLineEditServerSettingsDescriptionText WRITE setLineEditServerSettingsDescriptionText NOTIFY lineEditServerSettingsDescriptionTextChanged)
    Q_PROPERTY(QString labelServerSettingsCurrentVpnProtocolText READ getLabelServerSettingsCurrentVpnProtocolText WRITE setLabelServerSettingsCurrentVpnProtocolText NOTIFY labelServerSettingsCurrentVpnProtocolTextChanged)

    Q_INVOKABLE void onPushButtonServerSettingsClearServer();
    Q_INVOKABLE void onPushButtonServerSettingsForgetServer();
    Q_INVOKABLE void onPushButtonServerSettingsShareFullClicked();
    Q_INVOKABLE void onPushButtonServerSettingsClearClientCacheClicked();
    Q_INVOKABLE void onLineEditServerSettingsDescriptionEditingFinished();

public:
    explicit ServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerSettingsLogic() = default;

    bool getPageServerSettingsEnabled() const;
    void setPageServerSettingsEnabled(bool pageServerSettingsEnabled);

    bool getLabelServerSettingsWaitInfoVisible() const;
    void setLabelServerSettingsWaitInfoVisible(bool labelServerSettingsWaitInfoVisible);
    QString getLabelServerSettingsWaitInfoText() const;
    void setLabelServerSettingsWaitInfoText(const QString &labelServerSettingsWaitInfoText);
    bool getPushButtonServerSettingsClearVisible() const;
    void setPushButtonServerSettingsClearVisible(bool pushButtonServerSettingsClearVisible);
    bool getPushButtonServerSettingsClearClientCacheVisible() const;
    void setPushButtonServerSettingsClearClientCacheVisible(bool pushButtonServerSettingsClearClientCacheVisible);
    bool getPushButtonServerSettingsShareFullVisible() const;
    void setPushButtonServerSettingsShareFullVisible(bool pushButtonServerSettingsShareFullVisible);
    QString getLabelServerSettingsServerText() const;
    void setLabelServerSettingsServerText(const QString &labelServerSettingsServerText);
    QString getLineEditServerSettingsDescriptionText() const;
    void setLineEditServerSettingsDescriptionText(const QString &lineEditServerSettingsDescriptionText);
    QString getLabelServerSettingsCurrentVpnProtocolText() const;
    void setLabelServerSettingsCurrentVpnProtocolText(const QString &labelServerSettingsCurrentVpnProtocolText);

    QString getPushButtonServerSettingsClearText() const;
    void setPushButtonServerSettingsClearText(const QString &pushButtonServerSettingsClearText);
    QString getPushButtonServerSettingsClearClientCacheText() const;
    void setPushButtonServerSettingsClearClientCacheText(const QString &pushButtonServerSettingsClearClientCacheText);

signals:
    void pageServerSettingsEnabledChanged();
    void labelServerSettingsWaitInfoVisibleChanged();
    void labelServerSettingsWaitInfoTextChanged();
    void pushButtonServerSettingsClearTextChanged();
    void pushButtonServerSettingsClearVisibleChanged();
    void pushButtonServerSettingsClearClientCacheVisibleChanged();
    void pushButtonServerSettingsShareFullVisibleChanged();
    void pushButtonServerSettingsClearClientCacheTextChanged();
    void labelServerSettingsServerTextChanged();
    void lineEditServerSettingsDescriptionTextChanged();
    void labelServerSettingsCurrentVpnProtocolTextChanged();

private:


private slots:


private:
    bool m_pageServerSettingsEnabled;
    bool m_labelServerSettingsWaitInfoVisible;
    bool m_pushButtonServerSettingsClearVisible;
    bool m_pushButtonServerSettingsClearClientCacheVisible;
    bool m_pushButtonServerSettingsShareFullVisible;

    QString m_lineEditServerSettingsDescriptionText;
    QString m_labelServerSettingsCurrentVpnProtocolText;
    QString m_labelServerSettingsServerText;
    QString m_labelServerSettingsWaitInfoText;
    QString m_pushButtonServerSettingsClearText;
    QString m_pushButtonServerSettingsClearClientCacheText;

};
#endif // SERVER_SETTINGS_LOGIC_H

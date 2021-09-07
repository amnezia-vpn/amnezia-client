#ifndef VPN_LOGIC_H
#define VPN_LOGIC_H

#include "PageLogicBase.h"
#include "protocols/vpnprotocol.h"

class UiLogic;

class VpnLogic : public PageLogicBase
{
    Q_OBJECT

    Q_PROPERTY(bool pushButtonConnectChecked READ getPushButtonConnectChecked WRITE setPushButtonConnectChecked NOTIFY pushButtonConnectCheckedChanged)
    Q_PROPERTY(QString labelSpeedReceivedText READ getLabelSpeedReceivedText WRITE setLabelSpeedReceivedText NOTIFY labelSpeedReceivedTextChanged)
    Q_PROPERTY(QString labelSpeedSentText READ getLabelSpeedSentText WRITE setLabelSpeedSentText NOTIFY labelSpeedSentTextChanged)
    Q_PROPERTY(QString labelStateText READ getLabelStateText WRITE setLabelStateText NOTIFY labelStateTextChanged)
    Q_PROPERTY(bool pushButtonConnectEnabled READ getPushButtonConnectEnabled WRITE setPushButtonConnectEnabled NOTIFY pushButtonConnectEnabledChanged)
    Q_PROPERTY(bool widgetVpnModeEnabled READ getWidgetVpnModeEnabled WRITE setWidgetVpnModeEnabled NOTIFY widgetVpnModeEnabledChanged)
    Q_PROPERTY(QString labelErrorText READ getLabelErrorText WRITE setLabelErrorText NOTIFY labelErrorTextChanged)
    Q_PROPERTY(bool pushButtonVpnAddSiteEnabled READ getPushButtonVpnAddSiteEnabled WRITE setPushButtonVpnAddSiteEnabled NOTIFY pushButtonVpnAddSiteEnabledChanged)

    Q_PROPERTY(bool radioButtonVpnModeAllSitesChecked READ getRadioButtonVpnModeAllSitesChecked WRITE setRadioButtonVpnModeAllSitesChecked NOTIFY radioButtonVpnModeAllSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeForwardSitesChecked READ getRadioButtonVpnModeForwardSitesChecked WRITE setRadioButtonVpnModeForwardSitesChecked NOTIFY radioButtonVpnModeForwardSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeExceptSitesChecked READ getRadioButtonVpnModeExceptSitesChecked WRITE setRadioButtonVpnModeExceptSitesChecked NOTIFY radioButtonVpnModeExceptSitesCheckedChanged)

public:
    Q_INVOKABLE void updateVpnPage();

    Q_INVOKABLE void onRadioButtonVpnModeAllSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeForwardSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeExceptSitesToggled(bool checked);

    Q_INVOKABLE void onPushButtonConnectClicked(bool checked);

public:
    explicit VpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~VpnLogic() = default;

    bool getPushButtonConnectChecked() const;
    void setPushButtonConnectChecked(bool pushButtonConnectChecked);


    QString getLabelSpeedReceivedText() const;
    void setLabelSpeedReceivedText(const QString &labelSpeedReceivedText);
    QString getLabelSpeedSentText() const;
    void setLabelSpeedSentText(const QString &labelSpeedSentText);
    QString getLabelStateText() const;
    void setLabelStateText(const QString &labelStateText);
    bool getPushButtonConnectEnabled() const;
    void setPushButtonConnectEnabled(bool pushButtonConnectEnabled);
    bool getWidgetVpnModeEnabled() const;
    void setWidgetVpnModeEnabled(bool widgetVpnModeEnabled);
    QString getLabelErrorText() const;
    void setLabelErrorText(const QString &labelErrorText);

    bool getRadioButtonVpnModeAllSitesChecked() const;
    void setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked);
    bool getRadioButtonVpnModeForwardSitesChecked() const;
    void setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked);
    bool getRadioButtonVpnModeExceptSitesChecked() const;
    void setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked);
    bool getPushButtonVpnAddSiteEnabled() const;
    void setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled);

public slots:
    void onConnect();
    void onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void onDisconnect();

    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    void onVpnProtocolError(amnezia::ErrorCode errorCode);

signals:
    void radioButtonVpnModeAllSitesCheckedChanged();
    void radioButtonVpnModeForwardSitesCheckedChanged();
    void radioButtonVpnModeExceptSitesCheckedChanged();
    void pushButtonVpnAddSiteEnabledChanged();

    void pushButtonConnectCheckedChanged();

    void labelSpeedReceivedTextChanged();
    void labelSpeedSentTextChanged();
    void labelStateTextChanged();
    void pushButtonConnectEnabledChanged();
    void widgetVpnModeEnabledChanged();
    void labelErrorTextChanged();

private:
    bool m_pushButtonConnectChecked;

    bool m_radioButtonVpnModeAllSitesChecked;
    bool m_radioButtonVpnModeForwardSitesChecked;
    bool m_radioButtonVpnModeExceptSitesChecked;
    bool m_pushButtonVpnAddSiteEnabled;
    QString m_labelSpeedReceivedText;
    QString m_labelSpeedSentText;
    QString m_labelStateText;
    bool m_pushButtonConnectEnabled;
    bool m_widgetVpnModeEnabled;
    QString m_labelErrorText;

};
#endif // VPN_LOGIC_H

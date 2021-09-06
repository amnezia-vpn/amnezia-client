#ifndef OPENVPN_LOGIC_H
#define OPENVPN_LOGIC_H

#include "../../pages.h"
#include "settings.h"

class UiLogic;

class OpenVpnLogic : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(QString lineEditProtoOpenvpnSubnetText READ getLineEditProtoOpenvpnSubnetText WRITE setLineEditProtoOpenvpnSubnetText NOTIFY lineEditProtoOpenvpnSubnetTextChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnUdpChecked READ getRadioButtonProtoOpenvpnUdpChecked WRITE setRadioButtonProtoOpenvpnUdpChecked NOTIFY radioButtonProtoOpenvpnUdpCheckedChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnAutoEncryptionChecked READ getCheckBoxProtoOpenvpnAutoEncryptionChecked WRITE setCheckBoxProtoOpenvpnAutoEncryptionChecked NOTIFY checkBoxProtoOpenvpnAutoEncryptionCheckedChanged)
    Q_PROPERTY(QString comboBoxProtoOpenvpnCipherText READ getComboBoxProtoOpenvpnCipherText WRITE setComboBoxProtoOpenvpnCipherText NOTIFY comboBoxProtoOpenvpnCipherTextChanged)
    Q_PROPERTY(QString comboBoxProtoOpenvpnHashText READ getComboBoxProtoOpenvpnHashText WRITE setComboBoxProtoOpenvpnHashText NOTIFY comboBoxProtoOpenvpnHashTextChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnBlockDnsChecked READ getCheckBoxProtoOpenvpnBlockDnsChecked WRITE setCheckBoxProtoOpenvpnBlockDnsChecked NOTIFY checkBoxProtoOpenvpnBlockDnsCheckedChanged)
    Q_PROPERTY(QString lineEditProtoOpenvpnPortText READ getLineEditProtoOpenvpnPortText WRITE setLineEditProtoOpenvpnPortText NOTIFY lineEditProtoOpenvpnPortTextChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnTlsAuthChecked READ getCheckBoxProtoOpenvpnTlsAuthChecked WRITE setCheckBoxProtoOpenvpnTlsAuthChecked NOTIFY checkBoxProtoOpenvpnTlsAuthCheckedChanged)

    Q_PROPERTY(bool widgetProtoOpenvpnEnabled READ getWidgetProtoOpenvpnEnabled WRITE setWidgetProtoOpenvpnEnabled NOTIFY widgetProtoOpenvpnEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnSaveVisible READ getPushButtonProtoOpenvpnSaveVisible WRITE setPushButtonProtoOpenvpnSaveVisible NOTIFY pushButtonProtoOpenvpnSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoOpenvpnResetVisible READ getProgressBarProtoOpenvpnResetVisible WRITE setProgressBarProtoOpenvpnResetVisible NOTIFY progressBarProtoOpenvpnResetVisibleChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnUdpEnabled READ getRadioButtonProtoOpenvpnUdpEnabled WRITE setRadioButtonProtoOpenvpnUdpEnabled NOTIFY radioButtonProtoOpenvpnUdpEnabledChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnTcpEnabled READ getRadioButtonProtoOpenvpnTcpEnabled WRITE setRadioButtonProtoOpenvpnTcpEnabled NOTIFY radioButtonProtoOpenvpnTcpEnabledChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnTcpChecked READ getRadioButtonProtoOpenvpnTcpChecked WRITE setRadioButtonProtoOpenvpnTcpChecked NOTIFY radioButtonProtoOpenvpnTcpCheckedChanged)
    Q_PROPERTY(bool lineEditProtoOpenvpnPortEnabled READ getLineEditProtoOpenvpnPortEnabled WRITE setLineEditProtoOpenvpnPortEnabled NOTIFY lineEditProtoOpenvpnPortEnabledChanged)

    Q_PROPERTY(bool comboBoxProtoOpenvpnCipherEnabled READ getComboBoxProtoOpenvpnCipherEnabled WRITE setComboBoxProtoOpenvpnCipherEnabled NOTIFY comboBoxProtoOpenvpnCipherEnabledChanged)
    Q_PROPERTY(bool comboBoxProtoOpenvpnHashEnabled READ getComboBoxProtoOpenvpnHashEnabled WRITE setComboBoxProtoOpenvpnHashEnabled NOTIFY comboBoxProtoOpenvpnHashEnabledChanged)
    Q_PROPERTY(bool pageProtoOpenvpnEnabled READ getPageProtoOpenvpnEnabled WRITE setPageProtoOpenvpnEnabled NOTIFY pageProtoOpenvpnEnabledChanged)
    Q_PROPERTY(bool labelProtoOpenvpnInfoVisible READ getLabelProtoOpenvpnInfoVisible WRITE setLabelProtoOpenvpnInfoVisible NOTIFY labelProtoOpenvpnInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoOpenvpnInfoText READ getLabelProtoOpenvpnInfoText WRITE setLabelProtoOpenvpnInfoText NOTIFY labelProtoOpenvpnInfoTextChanged)
    Q_PROPERTY(int progressBarProtoOpenvpnResetValue READ getProgressBarProtoOpenvpnResetValue WRITE setProgressBarProtoOpenvpnResetValue NOTIFY progressBarProtoOpenvpnResetValueChanged)
    Q_PROPERTY(int progressBarProtoOpenvpnResetMaximium READ getProgressBarProtoOpenvpnResetMaximium WRITE setProgressBarProtoOpenvpnResetMaximium NOTIFY progressBarProtoOpenvpnResetMaximiumChanged)

public:
    explicit OpenVpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OpenVpnLogic() = default;

    void updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getOpenVpnConfigFromPage(QJsonObject oldConfig);

    QString getLineEditProtoOpenvpnSubnetText() const;
    void setLineEditProtoOpenvpnSubnetText(const QString &lineEditProtoOpenvpnSubnetText);
    bool getRadioButtonProtoOpenvpnUdpChecked() const;
    void setRadioButtonProtoOpenvpnUdpChecked(bool radioButtonProtoOpenvpnUdpChecked);
    bool getCheckBoxProtoOpenvpnAutoEncryptionChecked() const;
    void setCheckBoxProtoOpenvpnAutoEncryptionChecked(bool checkBoxProtoOpenvpnAutoEncryptionChecked);
    QString getComboBoxProtoOpenvpnCipherText() const;
    void setComboBoxProtoOpenvpnCipherText(const QString &comboBoxProtoOpenvpnCipherText);
    QString getComboBoxProtoOpenvpnHashText() const;
    void setComboBoxProtoOpenvpnHashText(const QString &comboBoxProtoOpenvpnHashText);
    bool getCheckBoxProtoOpenvpnBlockDnsChecked() const;
    void setCheckBoxProtoOpenvpnBlockDnsChecked(bool checkBoxProtoOpenvpnBlockDnsChecked);
    QString getLineEditProtoOpenvpnPortText() const;
    void setLineEditProtoOpenvpnPortText(const QString &lineEditProtoOpenvpnPortText);
    bool getCheckBoxProtoOpenvpnTlsAuthChecked() const;
    void setCheckBoxProtoOpenvpnTlsAuthChecked(bool checkBoxProtoOpenvpnTlsAuthChecked);


    bool getWidgetProtoOpenvpnEnabled() const;
    void setWidgetProtoOpenvpnEnabled(bool widgetProtoOpenvpnEnabled);
    bool getPushButtonProtoOpenvpnSaveVisible() const;
    void setPushButtonProtoOpenvpnSaveVisible(bool pushButtonProtoOpenvpnSaveVisible);
    bool getProgressBarProtoOpenvpnResetVisible() const;
    void setProgressBarProtoOpenvpnResetVisible(bool progressBarProtoOpenvpnResetVisible);
    bool getRadioButtonProtoOpenvpnUdpEnabled() const;
    void setRadioButtonProtoOpenvpnUdpEnabled(bool radioButtonProtoOpenvpnUdpEnabled);
    bool getRadioButtonProtoOpenvpnTcpEnabled() const;
    void setRadioButtonProtoOpenvpnTcpEnabled(bool radioButtonProtoOpenvpnTcpEnabled);
    bool getRadioButtonProtoOpenvpnTcpChecked() const;
    void setRadioButtonProtoOpenvpnTcpChecked(bool radioButtonProtoOpenvpnTcpChecked);
    bool getLineEditProtoOpenvpnPortEnabled() const;
    void setLineEditProtoOpenvpnPortEnabled(bool lineEditProtoOpenvpnPortEnabled);

    bool getComboBoxProtoOpenvpnCipherEnabled() const;
    void setComboBoxProtoOpenvpnCipherEnabled(bool comboBoxProtoOpenvpnCipherEnabled);
    bool getComboBoxProtoOpenvpnHashEnabled() const;
    void setComboBoxProtoOpenvpnHashEnabled(bool comboBoxProtoOpenvpnHashEnabled);
    bool getPageProtoOpenvpnEnabled() const;
    void setPageProtoOpenvpnEnabled(bool pageProtoOpenvpnEnabled);
    bool getLabelProtoOpenvpnInfoVisible() const;
    void setLabelProtoOpenvpnInfoVisible(bool labelProtoOpenvpnInfoVisible);
    QString getLabelProtoOpenvpnInfoText() const;
    void setLabelProtoOpenvpnInfoText(const QString &labelProtoOpenvpnInfoText);
    int getProgressBarProtoOpenvpnResetValue() const;
    void setProgressBarProtoOpenvpnResetValue(int progressBarProtoOpenvpnResetValue);
    int getProgressBarProtoOpenvpnResetMaximium() const;
    void setProgressBarProtoOpenvpnResetMaximium(int progressBarProtoOpenvpnResetMaximium);

    Q_INVOKABLE void onCheckBoxProtoOpenvpnAutoEncryptionClicked();
    Q_INVOKABLE void onPushButtonProtoOpenvpnSaveClicked();

signals:
    void lineEditProtoOpenvpnSubnetTextChanged();
    void radioButtonProtoOpenvpnUdpCheckedChanged();
    void checkBoxProtoOpenvpnAutoEncryptionCheckedChanged();
    void comboBoxProtoOpenvpnCipherTextChanged();
    void comboBoxProtoOpenvpnHashTextChanged();
    void checkBoxProtoOpenvpnBlockDnsCheckedChanged();
    void lineEditProtoOpenvpnPortTextChanged();
    void checkBoxProtoOpenvpnTlsAuthCheckedChanged();
    void widgetProtoOpenvpnEnabledChanged();
    void pushButtonProtoOpenvpnSaveVisibleChanged();
    void progressBarProtoOpenvpnResetVisibleChanged();
    void radioButtonProtoOpenvpnUdpEnabledChanged();
    void radioButtonProtoOpenvpnTcpEnabledChanged();
    void radioButtonProtoOpenvpnTcpCheckedChanged();
    void lineEditProtoOpenvpnPortEnabledChanged();
    void comboBoxProtoOpenvpnCipherEnabledChanged();
    void comboBoxProtoOpenvpnHashEnabledChanged();
    void pageProtoOpenvpnEnabledChanged();
    void labelProtoOpenvpnInfoVisibleChanged();
    void labelProtoOpenvpnInfoTextChanged();
    void progressBarProtoOpenvpnResetValueChanged();
    void progressBarProtoOpenvpnResetMaximiumChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    QString m_lineEditProtoOpenvpnSubnetText;
    bool m_radioButtonProtoOpenvpnUdpChecked;
    bool m_checkBoxProtoOpenvpnAutoEncryptionChecked;
    QString m_comboBoxProtoOpenvpnCipherText;
    QString m_comboBoxProtoOpenvpnHashText;
    bool m_checkBoxProtoOpenvpnBlockDnsChecked;
    QString m_lineEditProtoOpenvpnPortText;
    bool m_checkBoxProtoOpenvpnTlsAuthChecked;
    bool m_widgetProtoOpenvpnEnabled;
    bool m_pushButtonProtoOpenvpnSaveVisible;
    bool m_progressBarProtoOpenvpnResetVisible;
    bool m_radioButtonProtoOpenvpnUdpEnabled;
    bool m_radioButtonProtoOpenvpnTcpEnabled;
    bool m_radioButtonProtoOpenvpnTcpChecked;
    bool m_lineEditProtoOpenvpnPortEnabled;
    bool m_comboBoxProtoOpenvpnCipherEnabled;
    bool m_comboBoxProtoOpenvpnHashEnabled;
    bool m_pageProtoOpenvpnEnabled;
    bool m_labelProtoOpenvpnInfoVisible;
    QString m_labelProtoOpenvpnInfoText;
    int m_progressBarProtoOpenvpnResetValue;
    int m_progressBarProtoOpenvpnResetMaximium;
};
#endif // OPENVPN_LOGIC_H

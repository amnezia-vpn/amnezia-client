#ifndef SHADOWSOCKS_LOGIC_H
#define SHADOWSOCKS_LOGIC_H

#include "../../pages.h"
#include "settings.h"

class UiLogic;

class ShadowSocksLogic : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool widgetProtoSsEnabled READ getWidgetProtoSsEnabled WRITE setWidgetProtoSsEnabled NOTIFY widgetProtoSsEnabledChanged)
    Q_PROPERTY(QString comboBoxProtoShadowsocksCipherText READ getComboBoxProtoShadowsocksCipherText WRITE setComboBoxProtoShadowsocksCipherText NOTIFY comboBoxProtoShadowsocksCipherTextChanged)
    Q_PROPERTY(QString lineEditProtoShadowsocksPortText READ getLineEditProtoShadowsocksPortText WRITE setLineEditProtoShadowsocksPortText NOTIFY lineEditProtoShadowsocksPortTextChanged)
    Q_PROPERTY(bool pushButtonProtoShadowsocksSaveVisible READ getPushButtonProtoShadowsocksSaveVisible WRITE setPushButtonProtoShadowsocksSaveVisible NOTIFY pushButtonProtoShadowsocksSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoShadowsocksResetVisible READ getProgressBarProtoShadowsocksResetVisible WRITE setProgressBarProtoShadowsocksResetVisible NOTIFY progressBarProtoShadowsocksResetVisibleChanged)
    Q_PROPERTY(bool lineEditProtoShadowsocksPortEnabled READ getLineEditProtoShadowsocksPortEnabled WRITE setLineEditProtoShadowsocksPortEnabled NOTIFY lineEditProtoShadowsocksPortEnabledChanged)
    Q_PROPERTY(bool pageProtoShadowsocksEnabled READ getPageProtoShadowsocksEnabled WRITE setPageProtoShadowsocksEnabled NOTIFY pageProtoShadowsocksEnabledChanged)
    Q_PROPERTY(bool labelProtoShadowsocksInfoVisible READ getLabelProtoShadowsocksInfoVisible WRITE setLabelProtoShadowsocksInfoVisible NOTIFY labelProtoShadowsocksInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoShadowsocksInfoText READ getLabelProtoShadowsocksInfoText WRITE setLabelProtoShadowsocksInfoText NOTIFY labelProtoShadowsocksInfoTextChanged)
    Q_PROPERTY(int progressBarProtoShadowsocksResetValue READ getProgressBarProtoShadowsocksResetValue WRITE setProgressBarProtoShadowsocksResetValue NOTIFY progressBarProtoShadowsocksResetValueChanged)
    Q_PROPERTY(int progressBarProtoShadowsocksResetMaximium READ getProgressBarProtoShadowsocksResetMaximium WRITE setProgressBarProtoShadowsocksResetMaximium NOTIFY progressBarProtoShadowsocksResetMaximiumChanged)

    Q_INVOKABLE void onPushButtonProtoShadowsocksSaveClicked();

public:
    explicit ShadowSocksLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShadowSocksLogic() = default;

    void updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getShadowSocksConfigFromPage(QJsonObject oldConfig);

    bool getWidgetProtoSsEnabled() const;
    void setWidgetProtoSsEnabled(bool widgetProtoSsEnabled);
    QString getComboBoxProtoShadowsocksCipherText() const;
    void setComboBoxProtoShadowsocksCipherText(const QString &comboBoxProtoShadowsocksCipherText);
    QString getLineEditProtoShadowsocksPortText() const;
    void setLineEditProtoShadowsocksPortText(const QString &lineEditProtoShadowsocksPortText);

    bool getPushButtonProtoShadowsocksSaveVisible() const;
    void setPushButtonProtoShadowsocksSaveVisible(bool pushButtonProtoShadowsocksSaveVisible);
    bool getProgressBarProtoShadowsocksResetVisible() const;
    void setProgressBarProtoShadowsocksResetVisible(bool progressBarProtoShadowsocksResetVisible);
    bool getLineEditProtoShadowsocksPortEnabled() const;
    void setLineEditProtoShadowsocksPortEnabled(bool lineEditProtoShadowsocksPortEnabled);

    bool getPageProtoShadowsocksEnabled() const;
    void setPageProtoShadowsocksEnabled(bool pageProtoShadowsocksEnabled);
    bool getLabelProtoShadowsocksInfoVisible() const;
    void setLabelProtoShadowsocksInfoVisible(bool labelProtoShadowsocksInfoVisible);
    QString getLabelProtoShadowsocksInfoText() const;
    void setLabelProtoShadowsocksInfoText(const QString &labelProtoShadowsocksInfoText);
    int getProgressBarProtoShadowsocksResetValue() const;
    void setProgressBarProtoShadowsocksResetValue(int progressBarProtoShadowsocksResetValue);
    int getProgressBarProtoShadowsocksResetMaximium() const;
    void setProgressBarProtoShadowsocksResetMaximium(int progressBarProtoShadowsocksResetMaximium);

signals:
    void widgetProtoSsEnabledChanged();
    void comboBoxProtoShadowsocksCipherTextChanged();
    void lineEditProtoShadowsocksPortTextChanged();
    void pushButtonProtoShadowsocksSaveVisibleChanged();
    void progressBarProtoShadowsocksResetVisibleChanged();
    void lineEditProtoShadowsocksPortEnabledChanged();
    void pageProtoShadowsocksEnabledChanged();
    void labelProtoShadowsocksInfoVisibleChanged();
    void labelProtoShadowsocksInfoTextChanged();
    void progressBarProtoShadowsocksResetValueChanged();
    void progressBarProtoShadowsocksResetMaximiumChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    bool m_widgetProtoSsEnabled;
    QString m_comboBoxProtoShadowsocksCipherText;
    QString m_lineEditProtoShadowsocksPortText;
    bool m_pushButtonProtoShadowsocksSaveVisible;
    bool m_progressBarProtoShadowsocksResetVisible;
    bool m_lineEditProtoShadowsocksPortEnabled;
    bool m_pageProtoShadowsocksEnabled;
    bool m_labelProtoShadowsocksInfoVisible;
    QString m_labelProtoShadowsocksInfoText;
    int m_progressBarProtoShadowsocksResetValue;
    int m_progressBarProtoShadowsocksResetMaximium;

};
#endif // SHADOWSOCKS_LOGIC_H

#ifndef CLOAK_LOGIC_H
#define CLOAK_LOGIC_H

#include "../PageLogicBase.h"

class UiLogic;

class CloakLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_PROPERTY(QString comboBoxProtoCloakCipherText READ getComboBoxProtoCloakCipherText WRITE setComboBoxProtoCloakCipherText NOTIFY comboBoxProtoCloakCipherTextChanged)
    Q_PROPERTY(QString lineEditProtoCloakSiteText READ getLineEditProtoCloakSiteText WRITE setLineEditProtoCloakSiteText NOTIFY lineEditProtoCloakSiteTextChanged)
    Q_PROPERTY(QString lineEditProtoCloakPortText READ getLineEditProtoCloakPortText WRITE setLineEditProtoCloakPortText NOTIFY lineEditProtoCloakPortTextChanged)
    Q_PROPERTY(bool widgetProtoCloakEnabled READ getWidgetProtoCloakEnabled WRITE setWidgetProtoCloakEnabled NOTIFY widgetProtoCloakEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoCloakSaveVisible READ getPushButtonProtoCloakSaveVisible WRITE setPushButtonProtoCloakSaveVisible NOTIFY pushButtonProtoCloakSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoCloakResetVisible READ getProgressBarProtoCloakResetVisible WRITE setProgressBarProtoCloakResetVisible NOTIFY progressBarProtoCloakResetVisibleChanged)
    Q_PROPERTY(bool lineEditProtoCloakPortEnabled READ getLineEditProtoCloakPortEnabled WRITE setLineEditProtoCloakPortEnabled NOTIFY lineEditProtoCloakPortEnabledChanged)
    Q_PROPERTY(bool pageProtoCloakEnabled READ getPageProtoCloakEnabled WRITE setPageProtoCloakEnabled NOTIFY pageProtoCloakEnabledChanged)
    Q_PROPERTY(bool labelProtoCloakInfoVisible READ getLabelProtoCloakInfoVisible WRITE setLabelProtoCloakInfoVisible NOTIFY labelProtoCloakInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoCloakInfoText READ getLabelProtoCloakInfoText WRITE setLabelProtoCloakInfoText NOTIFY labelProtoCloakInfoTextChanged)
    Q_PROPERTY(int progressBarProtoCloakResetValue READ getProgressBarProtoCloakResetValue WRITE setProgressBarProtoCloakResetValue NOTIFY progressBarProtoCloakResetValueChanged)
    Q_PROPERTY(int progressBarProtoCloakResetMaximium READ getProgressBarProtoCloakResetMaximium WRITE setProgressBarProtoCloakResetMaximium NOTIFY progressBarProtoCloakResetMaximiumChanged)

    Q_INVOKABLE void onPushButtonProtoCloakSaveClicked();
public:
    explicit CloakLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~CloakLogic() = default;

    void updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getCloakConfigFromPage(QJsonObject oldConfig);

    QString getComboBoxProtoCloakCipherText() const;
    void setComboBoxProtoCloakCipherText(const QString &comboBoxProtoCloakCipherText);
    QString getLineEditProtoCloakSiteText() const;
    void setLineEditProtoCloakSiteText(const QString &lineEditProtoCloakSiteText);
    QString getLineEditProtoCloakPortText() const;
    void setLineEditProtoCloakPortText(const QString &lineEditProtoCloakPortText);
    bool getWidgetProtoCloakEnabled() const;
    void setWidgetProtoCloakEnabled(bool widgetProtoCloakEnabled);
    bool getPushButtonProtoCloakSaveVisible() const;
    void setPushButtonProtoCloakSaveVisible(bool pushButtonProtoCloakSaveVisible);
    bool getProgressBarProtoCloakResetVisible() const;
    void setProgressBarProtoCloakResetVisible(bool progressBarProtoCloakResetVisible);
    bool getLineEditProtoCloakPortEnabled() const;
    void setLineEditProtoCloakPortEnabled(bool lineEditProtoCloakPortEnabled);
    bool getPageProtoCloakEnabled() const;
    void setPageProtoCloakEnabled(bool pageProtoCloakEnabled);
    bool getLabelProtoCloakInfoVisible() const;
    void setLabelProtoCloakInfoVisible(bool labelProtoCloakInfoVisible);
    QString getLabelProtoCloakInfoText() const;
    void setLabelProtoCloakInfoText(const QString &labelProtoCloakInfoText);
    int getProgressBarProtoCloakResetValue() const;
    void setProgressBarProtoCloakResetValue(int progressBarProtoCloakResetValue);
    int getProgressBarProtoCloakResetMaximium() const;
    void setProgressBarProtoCloakResetMaximium(int progressBarProtoCloakResetMaximium);

signals:
    void comboBoxProtoCloakCipherTextChanged();
    void lineEditProtoCloakSiteTextChanged();
    void lineEditProtoCloakPortTextChanged();
    void widgetProtoCloakEnabledChanged();
    void pushButtonProtoCloakSaveVisibleChanged();
    void progressBarProtoCloakResetVisibleChanged();
    void lineEditProtoCloakPortEnabledChanged();
    void pageProtoCloakEnabledChanged();
    void labelProtoCloakInfoVisibleChanged();
    void labelProtoCloakInfoTextChanged();
    void progressBarProtoCloakResetValueChanged();
    void progressBarProtoCloakResetMaximiumChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    QString m_comboBoxProtoCloakCipherText;
    QString m_lineEditProtoCloakSiteText;
    QString m_lineEditProtoCloakPortText;
    bool m_widgetProtoCloakEnabled;
    bool m_pushButtonProtoCloakSaveVisible;
    bool m_progressBarProtoCloakResetVisible;
    bool m_lineEditProtoCloakPortEnabled;
    bool m_pageProtoCloakEnabled;
    bool m_labelProtoCloakInfoVisible;
    QString m_labelProtoCloakInfoText;
    int m_progressBarProtoCloakResetValue;
    int m_progressBarProtoCloakResetMaximium;

};
#endif // CLOAK_LOGIC_H

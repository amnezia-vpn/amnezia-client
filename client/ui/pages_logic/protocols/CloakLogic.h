#ifndef CLOAK_LOGIC_H
#define CLOAK_LOGIC_H

#include "../PageLogicBase.h"

class UiLogic;

class CloakLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, comboBoxProtoCloakCipherText)
    AUTO_PROPERTY(QString, lineEditProtoCloakSiteText)
    AUTO_PROPERTY(QString, lineEditProtoCloakPortText)
    AUTO_PROPERTY(bool, widgetProtoCloakEnabled)
    AUTO_PROPERTY(bool, pushButtonCloakSaveVisible)
    AUTO_PROPERTY(bool, progressBarProtoCloakResetVisible)
    AUTO_PROPERTY(bool, lineEditProtoCloakPortEnabled)
    AUTO_PROPERTY(bool, pageProtoCloakEnabled)
    AUTO_PROPERTY(bool, labelProtoCloakInfoVisible)
    AUTO_PROPERTY(QString, labelProtoCloakInfoText)
    AUTO_PROPERTY(int, progressBarProtoCloakResetValue)
    AUTO_PROPERTY(int, progressBarProtoCloakResetMaximium)

public:
    Q_INVOKABLE void onPushButtonProtoCloakSaveClicked();

public:
    explicit CloakLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~CloakLogic() = default;

    void updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getCloakConfigFromPage(QJsonObject oldConfig);

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // CLOAK_LOGIC_H

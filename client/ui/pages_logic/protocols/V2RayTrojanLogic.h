#ifndef V2RAYTROJANLOGIC_H
#define V2RAYTROJANLOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class V2RayTrojanLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, lineEditServerPortEnabled)
    AUTO_PROPERTY(QString, lineEditServerPortText)

    AUTO_PROPERTY(bool, lineEditLocalPortEnabled)
    AUTO_PROPERTY(QString, lineEditLocalPortText)

    AUTO_PROPERTY(bool, labelInfoVisible)
    AUTO_PROPERTY(QString, labelInfoText)

    AUTO_PROPERTY(int, progressBarResetValue)
    AUTO_PROPERTY(int, progressBarResetMaximium)
    AUTO_PROPERTY(bool, progressBarResetVisible)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)

    AUTO_PROPERTY(bool, labelServerBusyVisible)
    AUTO_PROPERTY(QString, labelServerBusyText)

    AUTO_PROPERTY(bool, pushButtonSaveVisible)
    AUTO_PROPERTY(bool, pushButtonCancelVisible)

public:
    Q_INVOKABLE void onPushButtonSaveClicked();
    Q_INVOKABLE void onPushButtonCancelClicked();

public:
    explicit V2RayTrojanLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~V2RayTrojanLogic() = default;

    void updateProtocolPage(const QJsonObject &v2rayConfig, DockerContainer container, bool haveAuthData) override;
    QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    UiLogic *m_uiLogic;

};


#endif // V2RAYTROJANLOGIC_H

#ifndef V2RAYLOGIC_H
#define V2RAYLOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class V2RayLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, comboBoxCipherText)
    AUTO_PROPERTY(QString, lineEditPortText)
    AUTO_PROPERTY(bool, pushButtonSaveVisible)
    AUTO_PROPERTY(bool, progressBarResetVisible)
    AUTO_PROPERTY(bool, lineEditPortEnabled)
    AUTO_PROPERTY(bool, labelInfoVisible)
    AUTO_PROPERTY(QString, labelInfoText)
    AUTO_PROPERTY(int, progressBarResetValue)
    AUTO_PROPERTY(int, progressBarResetMaximium)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)

    AUTO_PROPERTY(bool, labelServerBusyVisible)
    AUTO_PROPERTY(QString, labelServerBusyText)

    AUTO_PROPERTY(bool, pushButtonCancelVisible)

public:
    Q_INVOKABLE void onPushButtonSaveClicked();
    Q_INVOKABLE void onPushButtonCancelClicked();

public:
    explicit V2RayLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~V2RayLogic() = default;

    void updateProtocolPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData) override;
    QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    UiLogic *m_uiLogic;

};


#endif // V2RAYLOGIC_H

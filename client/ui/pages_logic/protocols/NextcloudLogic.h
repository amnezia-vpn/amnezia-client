#ifndef NEXTCLOUDLOGIC_H
#define NEXTCLOUDLOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class NextcloudLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineAdminUserText)
    AUTO_PROPERTY(QString, lineAdminPasswordText)

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
    explicit NextcloudLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NextcloudLogic() = default;

    void updateProtocolPage(const QJsonObject &nextcloudConfig, DockerContainer container, bool haveAuthData) override;
    QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    UiLogic *m_uiLogic;

};


#endif // NEXTCLOUDLOGIC_H

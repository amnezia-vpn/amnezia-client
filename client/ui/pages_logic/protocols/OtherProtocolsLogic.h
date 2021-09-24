#ifndef OTHER_PROTOCOLS_LOGIC_H
#define OTHER_PROTOCOLS_LOGIC_H

#include "PageProtocolLogicBase.h"

#include <QProcess>

class UiLogic;

class OtherProtocolsLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, labelTftpUserNameText)
    AUTO_PROPERTY(QString, labelTftpPasswordText)
    AUTO_PROPERTY(QString, labelTftpPortText)
    AUTO_PROPERTY(bool, pushButtonSftpMountEnabled)
    AUTO_PROPERTY(bool, checkBoxSftpRestoreChecked)

    AUTO_PROPERTY(QString, labelTorWebSiteAddressText)


public:
    Q_INVOKABLE void onPushButtonSftpMountDriveClicked();
    Q_INVOKABLE void checkBoxSftpRestoreClicked();
public:
    explicit OtherProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OtherProtocolsLogic();

    void updateProtocolPage(const QJsonObject &config, DockerContainer container, bool haveAuthData) override;
    //QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    QList <QProcess *> m_sftpMpuntProcesses;

};
#endif // OTHER_PROTOCOLS_LOGIC_H

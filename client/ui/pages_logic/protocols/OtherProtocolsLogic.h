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
    UiLogic *m_uiLogic;

#ifdef AMNEZIA_DESKTOP
    QList <QProcess *> m_sftpMountProcesses;
#endif

#ifdef Q_OS_WINDOWS
    QString getNextDriverLetter() const;
#endif

};
#endif // OTHER_PROTOCOLS_LOGIC_H

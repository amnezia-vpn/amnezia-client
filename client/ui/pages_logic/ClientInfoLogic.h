#ifndef CLIENTINFOLOGIC_H
#define CLIENTINFOLOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ClientInfoLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditNameAliasText)
    AUTO_PROPERTY(QString, labelOpenVpnCertId)
    AUTO_PROPERTY(QString, textAreaOpenVpnCertData)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)
    AUTO_PROPERTY(QString, textAreaWireGuardKeyData)

public:
    ClientInfoLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ClientInfoLogic() = default;

    void setCurrentClientId(int index);

public slots:
    void onUpdatePage() override;
    void onLineEditNameAliasEditingFinished();
    void onRevokeOpenVpnCertificateClicked();
    void onRevokeWireGuardKeyClicked();

private:
    int m_currentClientIndex;
};

#endif // CLIENTINFOLOGIC_H

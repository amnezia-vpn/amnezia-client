#ifndef CLIENTINFOLOGIC_H
#define CLIENTINFOLOGIC_H

#include "PageLogicBase.h"

#include "core/defs.h"
#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"

class UiLogic;

class ClientInfoLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditNameAliasText)
    AUTO_PROPERTY(QString, labelOpenVpnCertId)
    AUTO_PROPERTY(QString, textAreaOpenVpnCertData)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)
    AUTO_PROPERTY(QString, textAreaWireGuardKeyData)
    AUTO_PROPERTY(bool, busyIndicatorIsRunning);

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
    ErrorCode setClientsList(const ServerCredentials &credentials, DockerContainer container, Proto mainProtocol, const QJsonObject &clietns);

    int m_currentClientIndex;
};

#endif // CLIENTINFOLOGIC_H

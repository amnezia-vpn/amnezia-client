#ifndef SERVER_CONTAINERS_LOGIC_H
#define SERVER_CONTAINERS_LOGIC_H

#include "PageLogicBase.h"
#include "containers/containers_defs.h"

class UiLogic;

class ServerContainersLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonProtoSettingsClicked(DockerContainer c, Proto p);
    Q_INVOKABLE void onPushButtonDefaultClicked(DockerContainer c);
    Q_INVOKABLE void onPushButtonShareClicked(DockerContainer c);
    Q_INVOKABLE void onPushButtonRemoveClicked(DockerContainer c);
    Q_INVOKABLE void onPushButtonContinueClicked(DockerContainer c, int port, TransportProto tp);

    AUTO_PROPERTY(bool, isManagedServer)

public:
    explicit ServerContainersLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerContainersLogic() = default;

};
#endif // SERVER_CONTAINERS_LOGIC_H

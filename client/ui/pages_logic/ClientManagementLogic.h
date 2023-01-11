#ifndef CLIENTMANAGMENTLOGIC_H
#define CLIENTMANAGMENTLOGIC_H

#include "PageLogicBase.h"

#include "protocols/protocols_defs.h"

class UiLogic;

class ClientManagementLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)
    AUTO_PROPERTY(bool, busyIndicatorIsRunning);

public:
    ClientManagementLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ClientManagementLogic() = default;

public slots:
    void onUpdatePage() override;
    void onClientItemClicked(int index);

private:
    amnezia::Proto m_currentMainProtocol;
};

#endif // CLIENTMANAGMENTLOGIC_H

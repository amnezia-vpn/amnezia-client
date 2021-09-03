#ifndef SERVER_VPN_PROTOCOLS_LOGIC_H
#define SERVER_VPN_PROTOCOLS_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class ServerVpnProtocolsLogic : public QObject
{
    Q_OBJECT

public:
    explicit ServerVpnProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerVpnProtocolsLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // SERVER_VPN_PROTOCOLS_LOGIC_H

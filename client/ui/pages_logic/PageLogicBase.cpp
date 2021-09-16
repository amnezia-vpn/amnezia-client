#include "PageLogicBase.h"


PageLogicBase::PageLogicBase(UiLogic *logic, QObject *parent):
    QObject(parent),
    m_pageEnabled{true},
    m_uiLogic(logic)
{

}

Page PageLogicBase::pageForProto(Protocol p)
{
    switch (p) {
    case Protocol::OpenVpn: return Page::OpenVpnSettings;
    case Protocol::ShadowSocks: return Page::ShadowSocksSettings;
    case Protocol::OpenVpn: return Page::OpenVpnSettings;
    case Protocol::OpenVpn: return Page::OpenVpnSettings;

    default:
        break;
    }
}

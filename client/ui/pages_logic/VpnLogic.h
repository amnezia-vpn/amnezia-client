#ifndef VPN_LOGIC_H
#define VPN_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class VpnLogic : public QObject
{
    Q_OBJECT

public:
    explicit VpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~VpnLogic() = default;

signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;
    UiLogic *uiLogic() const { return m_uiLogic; }



};
#endif // VPN_LOGIC_H

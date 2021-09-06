#ifndef SHADOWSOCKS_LOGIC_H
#define SHADOWSOCKS_LOGIC_H

#include "../../pages.h"
#include "settings.h"

class UiLogic;

class ShadowSocksLogic : public QObject
{
    Q_OBJECT

public:
    explicit ShadowSocksLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShadowSocksLogic() = default;

signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // SHADOWSOCKS_LOGIC_H

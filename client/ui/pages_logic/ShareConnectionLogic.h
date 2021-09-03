#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class ShareConnectionLogic: public QObject
{
    Q_OBJECT

public:
    explicit ShareConnectionLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShareConnectionLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // SHARE_CONNECTION_LOGIC_H

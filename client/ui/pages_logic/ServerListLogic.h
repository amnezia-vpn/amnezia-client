#ifndef SERVER_LIST_LOGIC_H
#define SERVER_LIST_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class ServerListLogic : public QObject
{
    Q_OBJECT

public:
    explicit ServerListLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerListLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;


};
#endif // SERVER_LIST_LOGIC_H

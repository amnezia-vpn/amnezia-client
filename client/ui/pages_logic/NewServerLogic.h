#ifndef NEW_SERVER_LOGIC_H
#define NEW_SERVER_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class NewServerLogic : public QObject
{
    Q_OBJECT

public:
    explicit NewServerLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // NEW_SERVER_LOGIC_H

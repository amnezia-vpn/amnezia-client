#ifndef SERVER_SETTINGS_LOGIC_H
#define SERVER_SETTINGS_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class ServerSettingsLogic : public QObject
{
    Q_OBJECT

public:
    explicit ServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerSettingsLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // SERVER_SETTINGS_LOGIC_H

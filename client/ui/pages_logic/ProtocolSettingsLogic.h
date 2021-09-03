#ifndef PROTOCOL_SETTINGS_LOGIC_H
#define PROTOCOL_SETTINGS_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class ProtocolSettingsLogic : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ProtocolSettingsLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // PROTOCOL_SETTINGS_LOGIC_H

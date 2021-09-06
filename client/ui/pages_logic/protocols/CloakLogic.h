#ifndef CLOAK_LOGIC_H
#define CLOAK_LOGIC_H

#include "../../pages.h"
#include "settings.h"

class UiLogic;

class CloakLogic : public QObject
{
    Q_OBJECT

public:
    explicit CloakLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~CloakLogic() = default;

signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // CLOAK_LOGIC_H

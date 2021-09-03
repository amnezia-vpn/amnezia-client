#ifndef WIZARD_LOGIC_H
#define WIZARD_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class WizardLogic : public QObject
{
    Q_OBJECT

public:
    explicit WizardLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~WizardLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // WIZARD_LOGIC_H

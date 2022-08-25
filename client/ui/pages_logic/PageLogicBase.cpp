#include "PageLogicBase.h"
#include "ui/uilogic.h"


PageLogicBase::PageLogicBase(UiLogic *logic, QObject *parent):
    QObject(parent),
    m_pageEnabled{true},
    m_uiLogic(logic)
{
    m_settings = logic->m_settings;
}



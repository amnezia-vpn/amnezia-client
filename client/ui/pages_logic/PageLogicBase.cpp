#include "PageLogicBase.h"


PageLogicBase::PageLogicBase(UiLogic *logic, QObject *parent):
    QObject(parent),
    m_pageEnabled{true},
    m_uiLogic(logic)
{

}

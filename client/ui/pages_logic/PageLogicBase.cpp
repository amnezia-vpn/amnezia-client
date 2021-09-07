#include "PageLogicBase.h"


PageLogicBase::PageLogicBase(UiLogic *logic, QObject *parent):
    QObject(parent),
    m_uiLogic(logic)
{

}

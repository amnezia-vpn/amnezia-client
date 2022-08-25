#include "PageLogicBase.h"

#include "ui/uilogic.h"
#include "settings.h"
#include "configurators/vpn_configurator.h"

PageLogicBase::PageLogicBase(UiLogic *logic, QObject *parent):
    QObject(parent),
    m_pageEnabled{true},
    m_uiLogic(logic)
{
    m_settings = logic->m_settings;
    m_configurator = logic->m_configurator;
    m_serverController = logic->m_serverController;
}



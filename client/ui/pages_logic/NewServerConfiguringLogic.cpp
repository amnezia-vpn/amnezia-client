#include "NewServerConfiguringLogic.h"

NewServerConfiguringLogic::NewServerConfiguringLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarValue{0},
    m_pageEnabled{true},
    m_labelWaitInfoVisible{true},
    m_labelWaitInfoText{tr("Please wait, configuring process may take up to 5 minutes")},
    m_progressBarVisible{true},
    m_progressBarMaximium{100},
    m_progressBarTextVisible{true},
    m_progressBarText{tr("Configuring...")}
{

}


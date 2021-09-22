#include "NetworkSettingsLogic.h"

#include "defines.h"
#include "utils.h"

NetworkSettingsLogic::NetworkSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_ipAddressValidatorRegex{Utils::ipAddressRegExp().pattern()}
{

}

void NetworkSettingsLogic::onUpdatePage()
{
    set_lineEditDns1Text(m_settings.primaryDns());
    set_lineEditDns2Text(m_settings.secondaryDns());
}

void NetworkSettingsLogic::onLineEditDns1EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setPrimaryDns(text);
    }
}

void NetworkSettingsLogic::onLineEditDns2EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setSecondaryDns(text);
    }
}

void NetworkSettingsLogic::onPushButtonResetDns1Clicked()
{
    m_settings.setPrimaryDns(m_settings.cloudFlareNs1);
    onUpdatePage();
}

void NetworkSettingsLogic::onPushButtonResetDns2Clicked()
{
    m_settings.setSecondaryDns(m_settings.cloudFlareNs2);
    onUpdatePage();
}

QString NetworkSettingsLogic::getIpAddressValidatorRegex() const
{
    return m_ipAddressValidatorRegex;
}

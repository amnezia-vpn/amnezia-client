#include "NetworkSettingsLogic.h"

#include "defines.h"
#include "utils.h"

using namespace amnezia;
using namespace PageEnumNS;

NetworkSettingsLogic::NetworkSettingsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_ipAddressValidatorRegex{Utils::ipAddressRegExp().pattern()}
{

}

void NetworkSettingsLogic::updateNetworkSettingsPage()
{
    setLineEditNetworkSettingsDns1Text(m_settings.primaryDns());
    setLineEditNetworkSettingsDns2Text(m_settings.secondaryDns());
}

QString NetworkSettingsLogic::getLineEditNetworkSettingsDns1Text() const
{
    return m_lineEditNetworkSettingsDns1Text;
}

void NetworkSettingsLogic::setLineEditNetworkSettingsDns1Text(const QString &lineEditNetworkSettingsDns1Text)
{
    if (m_lineEditNetworkSettingsDns1Text != lineEditNetworkSettingsDns1Text) {
        m_lineEditNetworkSettingsDns1Text = lineEditNetworkSettingsDns1Text;
        emit lineEditNetworkSettingsDns1TextChanged();
    }
}

QString NetworkSettingsLogic::getLineEditNetworkSettingsDns2Text() const
{
    return m_lineEditNetworkSettingsDns2Text;
}

void NetworkSettingsLogic::setLineEditNetworkSettingsDns2Text(const QString &lineEditNetworkSettingsDns2Text)
{
    if (m_lineEditNetworkSettingsDns2Text != lineEditNetworkSettingsDns2Text) {
        m_lineEditNetworkSettingsDns2Text = lineEditNetworkSettingsDns2Text;
        emit lineEditNetworkSettingsDns2TextChanged();
    }
}

void NetworkSettingsLogic::onLineEditNetworkSettingsDns1EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setPrimaryDns(text);
    }
}

void NetworkSettingsLogic::onLineEditNetworkSettingsDns2EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setSecondaryDns(text);
    }
}

void NetworkSettingsLogic::onPushButtonNetworkSettingsResetdns1Clicked()
{
    m_settings.setPrimaryDns(m_settings.cloudFlareNs1);
    updateNetworkSettingsPage();
}

void NetworkSettingsLogic::onPushButtonNetworkSettingsResetdns2Clicked()
{
    m_settings.setSecondaryDns(m_settings.cloudFlareNs2);
    updateNetworkSettingsPage();
}

QString NetworkSettingsLogic::getIpAddressValidatorRegex() const
{
    return m_ipAddressValidatorRegex;
}

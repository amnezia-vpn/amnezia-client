#include "CloakLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

CloakLogic::CloakLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_comboBoxProtoCloakCipherText{"chacha20-poly1305"},
    m_lineEditProtoCloakSiteText{"tile.openstreetmap.org"},
    m_lineEditProtoCloakPortText{},
    m_widgetProtoCloakEnabled{false},
    m_pushButtonProtoCloakSaveVisible{false},
    m_progressBarProtoCloakResetVisible{false},
    m_lineEditProtoCloakPortEnabled{false},
    m_pageProtoCloakEnabled{true},
    m_labelProtoCloakInfoVisible{true},
    m_labelProtoCloakInfoText{},
    m_progressBarProtoCloakResetValue{0},
    m_progressBarProtoCloakResetMaximium{100}
{

}

void CloakLogic::updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
{
    setWidgetProtoCloakEnabled(haveAuthData);
    setPushButtonProtoCloakSaveVisible(haveAuthData);
    setProgressBarProtoCloakResetVisible(haveAuthData);

    setComboBoxProtoCloakCipherText(ckConfig.value(config_key::cipher).
                                    toString(protocols::cloak::defaultCipher));

    setLineEditProtoCloakSiteText(ckConfig.value(config_key::site).
                                  toString(protocols::cloak::defaultRedirSite));

    setLineEditProtoCloakPortText(ckConfig.value(config_key::port).
                                  toString(protocols::cloak::defaultPort));

    setLineEditProtoCloakPortEnabled(container == DockerContainer::OpenVpnOverCloak);
}



QJsonObject CloakLogic::getCloakConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, getComboBoxProtoCloakCipherText());
    oldConfig.insert(config_key::site, getLineEditProtoCloakSiteText());
    oldConfig.insert(config_key::port, getLineEditProtoCloakPortText());

    return oldConfig;
}

QString CloakLogic::getComboBoxProtoCloakCipherText() const
{
    return m_comboBoxProtoCloakCipherText;
}

void CloakLogic::setComboBoxProtoCloakCipherText(const QString &comboBoxProtoCloakCipherText)
{
    if (m_comboBoxProtoCloakCipherText != comboBoxProtoCloakCipherText) {
        m_comboBoxProtoCloakCipherText = comboBoxProtoCloakCipherText;
        emit comboBoxProtoCloakCipherTextChanged();
    }
}

QString CloakLogic::getLineEditProtoCloakPortText() const
{
    return m_lineEditProtoCloakPortText;
}

void CloakLogic::setLineEditProtoCloakPortText(const QString &lineEditProtoCloakPortText)
{
    if (m_lineEditProtoCloakPortText != lineEditProtoCloakPortText) {
        m_lineEditProtoCloakPortText = lineEditProtoCloakPortText;
        emit lineEditProtoCloakPortTextChanged();
    }
}

QString CloakLogic::getLineEditProtoCloakSiteText() const
{
    return m_lineEditProtoCloakSiteText;
}

void CloakLogic::setLineEditProtoCloakSiteText(const QString &lineEditProtoCloakSiteText)
{
    if (m_lineEditProtoCloakSiteText != lineEditProtoCloakSiteText) {
        m_lineEditProtoCloakSiteText = lineEditProtoCloakSiteText;
        emit lineEditProtoCloakSiteTextChanged();
    }
}

bool CloakLogic::getWidgetProtoCloakEnabled() const
{
    return m_widgetProtoCloakEnabled;
}

void CloakLogic::setWidgetProtoCloakEnabled(bool widgetProtoCloakEnabled)
{
    if (m_widgetProtoCloakEnabled != widgetProtoCloakEnabled) {
        m_widgetProtoCloakEnabled = widgetProtoCloakEnabled;
        emit widgetProtoCloakEnabledChanged();
    }
}

bool CloakLogic::getPushButtonProtoCloakSaveVisible() const
{
    return m_pushButtonProtoCloakSaveVisible;
}

void CloakLogic::setPushButtonProtoCloakSaveVisible(bool pushButtonProtoCloakSaveVisible)
{
    if (m_pushButtonProtoCloakSaveVisible != pushButtonProtoCloakSaveVisible) {
        m_pushButtonProtoCloakSaveVisible = pushButtonProtoCloakSaveVisible;
        emit pushButtonProtoCloakSaveVisibleChanged();
    }
}

bool CloakLogic::getProgressBarProtoCloakResetVisible() const
{
    return m_progressBarProtoCloakResetVisible;
}

void CloakLogic::setProgressBarProtoCloakResetVisible(bool progressBarProtoCloakResetVisible)
{
    if (m_progressBarProtoCloakResetVisible != progressBarProtoCloakResetVisible) {
        m_progressBarProtoCloakResetVisible = progressBarProtoCloakResetVisible;
        emit progressBarProtoCloakResetVisibleChanged();
    }
}

bool CloakLogic::getLineEditProtoCloakPortEnabled() const
{
    return m_lineEditProtoCloakPortEnabled;
}

void CloakLogic::setLineEditProtoCloakPortEnabled(bool lineEditProtoCloakPortEnabled)
{
    if (m_lineEditProtoCloakPortEnabled != lineEditProtoCloakPortEnabled) {
        m_lineEditProtoCloakPortEnabled = lineEditProtoCloakPortEnabled;
        emit lineEditProtoCloakPortEnabledChanged();
    }
}

bool CloakLogic::getPageProtoCloakEnabled() const
{
    return m_pageProtoCloakEnabled;
}

void CloakLogic::setPageProtoCloakEnabled(bool pageProtoCloakEnabled)
{
    if (m_pageProtoCloakEnabled != pageProtoCloakEnabled) {
        m_pageProtoCloakEnabled = pageProtoCloakEnabled;
        emit pageProtoCloakEnabledChanged();
    }
}

bool CloakLogic::getLabelProtoCloakInfoVisible() const
{
    return m_labelProtoCloakInfoVisible;
}

void CloakLogic::setLabelProtoCloakInfoVisible(bool labelProtoCloakInfoVisible)
{
    if (m_labelProtoCloakInfoVisible != labelProtoCloakInfoVisible) {
        m_labelProtoCloakInfoVisible = labelProtoCloakInfoVisible;
        emit labelProtoCloakInfoVisibleChanged();
    }
}

QString CloakLogic::getLabelProtoCloakInfoText() const
{
    return m_labelProtoCloakInfoText;
}

void CloakLogic::setLabelProtoCloakInfoText(const QString &labelProtoCloakInfoText)
{
    if (m_labelProtoCloakInfoText != labelProtoCloakInfoText) {
        m_labelProtoCloakInfoText = labelProtoCloakInfoText;
        emit labelProtoCloakInfoTextChanged();
    }
}

int CloakLogic::getProgressBarProtoCloakResetValue() const
{
    return m_progressBarProtoCloakResetValue;
}

void CloakLogic::setProgressBarProtoCloakResetValue(int progressBarProtoCloakResetValue)
{
    if (m_progressBarProtoCloakResetValue != progressBarProtoCloakResetValue) {
        m_progressBarProtoCloakResetValue = progressBarProtoCloakResetValue;
        emit progressBarProtoCloakResetValueChanged();
    }
}

int CloakLogic::getProgressBarProtoCloakResetMaximium() const
{
    return m_progressBarProtoCloakResetMaximium;
}

void CloakLogic::setProgressBarProtoCloakResetMaximium(int progressBarProtoCloakResetMaximium)
{
    if (m_progressBarProtoCloakResetMaximium != progressBarProtoCloakResetMaximium) {
        m_progressBarProtoCloakResetMaximium = progressBarProtoCloakResetMaximium;
        emit progressBarProtoCloakResetMaximiumChanged();
    }
}


void CloakLogic::onPushButtonProtoCloakSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::Cloak);
    protocolConfig = getCloakConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::cloak, protocolConfig);

    UiLogic::PageFunc page_proto_cloak;
    page_proto_cloak.setEnabledFunc = [this] (bool enabled) -> void {
        setPageProtoCloakEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_cloak_save;
    pushButton_proto_cloak_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoCloakSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_cloak_info;
    label_proto_cloak_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoCloakInfoVisible(visible);
    };
    label_proto_cloak_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoCloakInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_cloak_reset;
    progressBar_proto_cloak_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoCloakResetVisible(visible);
    };
    progressBar_proto_cloak_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoCloakResetValue(value);
    };
    progressBar_proto_cloak_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoCloakResetValue();
    };
    progressBar_proto_cloak_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoCloakResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_cloak, progressBar_proto_cloak_reset,
    pushButton_proto_cloak_save, label_proto_cloak_info);

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }

    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}

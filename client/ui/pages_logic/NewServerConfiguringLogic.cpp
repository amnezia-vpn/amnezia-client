#include "NewServerConfiguringLogic.h"

NewServerConfiguringLogic::NewServerConfiguringLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarNewServerConfiguringValue{0},
    m_pageNewServerConfiguringEnabled{true},
    m_labelNewServerConfiguringWaitInfoVisible{true},
    m_labelNewServerConfiguringWaitInfoText{tr("Please wait, configuring process may take up to 5 minutes")},
    m_progressBarNewServerConfiguringVisible{true},
    m_progressBarNewServerConfiguringMaximium{100},
    m_progressBarNewServerConfiguringTextVisible{true},
    m_progressBarNewServerConfiguringText{tr("Configuring...")}

{

}

double NewServerConfiguringLogic::getProgressBarNewServerConfiguringValue() const
{
    return m_progressBarNewServerConfiguringValue;
}

void NewServerConfiguringLogic::setProgressBarNewServerConfiguringValue(double progressBarNewServerConfiguringValue)
{
    if (m_progressBarNewServerConfiguringValue != progressBarNewServerConfiguringValue) {
        m_progressBarNewServerConfiguringValue = progressBarNewServerConfiguringValue;
        emit progressBarNewServerConfiguringValueChanged();
    }
}

bool NewServerConfiguringLogic::getPageNewServerConfiguringEnabled() const
{
    return m_pageNewServerConfiguringEnabled;
}

void NewServerConfiguringLogic::setPageNewServerConfiguringEnabled(bool pageNewServerConfiguringEnabled)
{
    if (m_pageNewServerConfiguringEnabled != pageNewServerConfiguringEnabled) {
        m_pageNewServerConfiguringEnabled = pageNewServerConfiguringEnabled;
        emit pageNewServerConfiguringEnabledChanged();
    }
}

bool NewServerConfiguringLogic::getLabelNewServerConfiguringWaitInfoVisible() const
{
    return m_labelNewServerConfiguringWaitInfoVisible;
}

void NewServerConfiguringLogic::setLabelNewServerConfiguringWaitInfoVisible(bool labelNewServerConfiguringWaitInfoVisible)
{
    if (m_labelNewServerConfiguringWaitInfoVisible != labelNewServerConfiguringWaitInfoVisible) {
        m_labelNewServerConfiguringWaitInfoVisible = labelNewServerConfiguringWaitInfoVisible;
        emit labelNewServerConfiguringWaitInfoVisibleChanged();
    }
}

QString NewServerConfiguringLogic::getLabelNewServerConfiguringWaitInfoText() const
{
    return m_labelNewServerConfiguringWaitInfoText;
}

void NewServerConfiguringLogic::setLabelNewServerConfiguringWaitInfoText(const QString &labelNewServerConfiguringWaitInfoText)
{
    if (m_labelNewServerConfiguringWaitInfoText != labelNewServerConfiguringWaitInfoText) {
        m_labelNewServerConfiguringWaitInfoText = labelNewServerConfiguringWaitInfoText;
        emit labelNewServerConfiguringWaitInfoTextChanged();
    }
}

bool NewServerConfiguringLogic::getProgressBarNewServerConfiguringVisible() const
{
    return m_progressBarNewServerConfiguringVisible;
}

void NewServerConfiguringLogic::setProgressBarNewServerConfiguringVisible(bool progressBarNewServerConfiguringVisible)
{
    if (m_progressBarNewServerConfiguringVisible != progressBarNewServerConfiguringVisible) {
        m_progressBarNewServerConfiguringVisible = progressBarNewServerConfiguringVisible;
        emit progressBarNewServerConfiguringVisibleChanged();
    }
}

int NewServerConfiguringLogic::getProgressBarNewServerConfiguringMaximium() const
{
    return m_progressBarNewServerConfiguringMaximium;
}

void NewServerConfiguringLogic::setProgressBarNewServerConfiguringMaximium(int progressBarNewServerConfiguringMaximium)
{
    if (m_progressBarNewServerConfiguringMaximium != progressBarNewServerConfiguringMaximium) {
        m_progressBarNewServerConfiguringMaximium = progressBarNewServerConfiguringMaximium;
        emit progressBarNewServerConfiguringMaximiumChanged();
    }
}

bool NewServerConfiguringLogic::getProgressBarNewServerConfiguringTextVisible() const
{
    return m_progressBarNewServerConfiguringTextVisible;
}

void NewServerConfiguringLogic::setProgressBarNewServerConfiguringTextVisible(bool progressBarNewServerConfiguringTextVisible)
{
    if (m_progressBarNewServerConfiguringTextVisible != progressBarNewServerConfiguringTextVisible) {
        m_progressBarNewServerConfiguringTextVisible = progressBarNewServerConfiguringTextVisible;
        emit progressBarNewServerConfiguringTextVisibleChanged();
    }
}

QString NewServerConfiguringLogic::getProgressBarNewServerConfiguringText() const
{
    return m_progressBarNewServerConfiguringText;
}

void NewServerConfiguringLogic::setProgressBarNewServerConfiguringText(const QString &progressBarNewServerConfiguringText)
{
    if (m_progressBarNewServerConfiguringText != progressBarNewServerConfiguringText) {
        m_progressBarNewServerConfiguringText = progressBarNewServerConfiguringText;
        emit progressBarNewServerConfiguringTextChanged();
    }
}

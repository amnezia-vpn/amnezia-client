#include "xraySettingsModel.h"

#include "logger.h"

#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayInboundValidator.h"
#include "ui/models/protocols/xrayPresetsModel.h"

using namespace amnezia;

namespace
{
    Logger logger("XraySettingsModel");

    QString issueText(const XrayInboundIssue &issue)
    {
        return QObject::tr("Line %1: %2").arg(issue.line).arg(issue.message);
    }
}

XraySettingsModel::XraySettingsModel(XrayPresetsModel *presetsModel, QObject *parent)
    : QObject(parent), m_presetsModel(presetsModel)
{
    connect(m_presetsModel, &XrayPresetsModel::presetsChanged, this, &XraySettingsModel::forgetRemovedPreset);
}

QString XraySettingsModel::presetId() const
{
    return m_protocolConfig.serverConfig.presetId;
}

QString XraySettingsModel::presetName() const
{
    const QString name = m_presetsModel->presetName(presetId());
    return name.isEmpty() ? tr("Custom config") : name;
}

bool XraySettingsModel::presetEdited() const
{
    const auto preset = m_presetsModel->preset(presetId());
    if (!preset) {
        return false;
    }
    return xrayInbound::normalized(preset->inbound) != xrayInbound::normalized(inboundText());
}

int XraySettingsModel::port() const
{
    return m_protocolConfig.serverConfig.port.toInt();
}

QString XraySettingsModel::inboundText() const
{
    return m_protocolConfig.serverConfig.inboundTemplate;
}

bool XraySettingsModel::hasUnsavedChanges() const
{
    const XrayServerConfig &current = m_protocolConfig.serverConfig;
    const XrayServerConfig &original = m_originalProtocolConfig.serverConfig;
    return current.presetId != original.presetId
            || xrayInbound::normalized(current.inboundTemplate) != xrayInbound::normalized(original.inboundTemplate);
}

bool XraySettingsModel::canReset() const
{
    return presetEdited();
}

QString XraySettingsModel::draftText() const
{
    return m_draft;
}

bool XraySettingsModel::draftModified() const
{
    return m_draft != inboundText();
}

bool XraySettingsModel::draftValid() const
{
    return m_draftValid;
}

int XraySettingsModel::draftErrorLine() const
{
    return m_draftErrorLine;
}

QString XraySettingsModel::draftStatus() const
{
    return m_draftStatus;
}

bool XraySettingsModel::draftHasError() const
{
    return !m_draftValid;
}

void XraySettingsModel::selectPreset(const QString &id)
{
    const auto preset = m_presetsModel->preset(id);
    if (!preset) {
        logger.warning() << "selectPreset: unknown preset" << id;
        return;
    }
    logger.info() << "selectPreset:" << presetId() << "->" << id << preset->name;
    m_protocolConfig.serverConfig.presetId = id;
    applyInbound(preset->inbound);
}

void XraySettingsModel::resetToPreset()
{
    const auto preset = m_presetsModel->preset(presetId());
    if (!preset) {
        logger.warning() << "resetToPreset: no preset for" << presetId() << ", restoring saved config";
        applyInbound(m_originalProtocolConfig.serverConfig.inboundTemplate);
        return;
    }
    logger.info() << "resetToPreset:" << preset->id << preset->name;
    applyInbound(preset->inbound);
}

void XraySettingsModel::openEditor()
{
    logger.info() << "openEditor: preset" << presetId() << "length" << QString::number(inboundText().size());
    m_draft = inboundText();
    refreshDraftValidation();
}

void XraySettingsModel::setDraft(const QString &text)
{
    if (text == m_draft) {
        return;
    }
    m_draft = text;
    refreshDraftValidation();
}

bool XraySettingsModel::formatDraft()
{
    const XrayInboundValidation validation = validateXrayInbound(m_draft);
    if (!validation.parsed) {
        logger.info() << "formatDraft: cannot format, JSON does not parse";
        return false;
    }
    m_draft = xrayInbound::format(validation.inbound);
    logger.info() << "formatDraft: formatted, length" << QString::number(m_draft.size());
    refreshDraftValidation();
    return true;
}

bool XraySettingsModel::commitDraft()
{
    if (!m_draftValid) {
        logger.warning() << "commitDraft: draft is invalid, line" << QString::number(m_draftErrorLine) << m_draftStatus;
        return false;
    }
    logger.info() << "commitDraft: modified" << (draftModified() ? "yes" : "no");
    if (draftModified()) {
        applyInbound(m_draft);
    }
    return true;
}

QString XraySettingsModel::saveAsPreset(const QString &name)
{
    const QString id = m_presetsModel->addPreset(name, inboundText());
    logger.info() << "saveAsPreset:" << name << "->" << id;
    selectPreset(id);
    return id;
}

QVariantMap XraySettingsModel::validate(const QString &text) const
{
    const XrayInboundValidation validation = validateXrayInbound(text);
    const XrayInboundIssue *error = validation.firstError();
    QVariantMap result;
    result.insert(QStringLiteral("valid"), validation.isValid());
    result.insert(QStringLiteral("line"), error ? error->line : 0);
    result.insert(QStringLiteral("message"), error ? issueText(*error) : QString());
    return result;
}

QStringList XraySettingsModel::validationErrors() const
{
    QStringList errors;
    for (const XrayInboundIssue &issue : validateXrayInbound(inboundText()).issues) {
        if (issue.level == XrayInboundIssue::Level::Error) {
            errors << issueText(issue);
        }
    }
    logger.info() << "validationErrors:" << QString::number(errors.size()) << errors.join(QStringLiteral(" | "));
    return errors;
}

bool XraySettingsModel::touchesServer() const
{
    const bool touches = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    logger.info() << "touchesServer:" << (touches ? "yes" : "no");
    return touches;
}

void XraySettingsModel::updateModel(DockerContainer container, const XrayProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    XrayServerConfig &srv = m_protocolConfig.serverConfig;

    if (srv.inboundTemplate.isEmpty() && !srv.isThirdPartyConfig) {
        srv.inboundTemplate = xrayInbound::templateFromServerConfig(srv);
        logger.info() << "updateModel: built template from typed settings";
    }
    m_presetsModel->setServer(srv.site, srv.port);
    if (srv.presetId.isEmpty()) {
        srv.presetId = xrayPresets::matchBuiltin(srv.inboundTemplate, srv.site, srv.port);
        logger.info() << "updateModel: preset id matched as"
                      << (srv.presetId.isEmpty() ? QStringLiteral("custom") : srv.presetId);
    }
    m_presetsModel->setSelectedId(srv.presetId);

    m_originalProtocolConfig = m_protocolConfig;
    m_draft = srv.inboundTemplate;
    refreshDraftValidation();

    logger.info() << "updateModel: container" << QString::number(static_cast<int>(container)) << "preset"
                  << srv.presetId << "port" << srv.port << "transport" << srv.transport << "security" << srv.security
                  << "flow" << srv.flow << "site" << srv.site << "thirdParty" << (srv.isThirdPartyConfig ? "yes" : "no");
    emit changed();
}

XrayProtocolConfig XraySettingsModel::getProtocolConfig()
{
    if (!m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig)) {
        m_protocolConfig.clearClientConfig();
        logger.info() << "getProtocolConfig: server settings changed, client config cleared";
    } else {
        logger.info() << "getProtocolConfig: server settings unchanged";
    }
    return m_protocolConfig;
}

void XraySettingsModel::applyInbound(const QString &text)
{
    XrayServerConfig updated = m_protocolConfig.serverConfig;
    if (!xrayInbound::applyTemplate(text, updated)) {
        logger.warning() << "applyInbound: template rejected, keeping previous settings";
        return;
    }
    m_protocolConfig.serverConfig = updated;
    m_presetsModel->setSelectedId(updated.presetId);
    m_draft = updated.inboundTemplate;
    refreshDraftValidation();
    logger.info() << "applyInbound: preset" << updated.presetId << "edited" << (presetEdited() ? "yes" : "no")
                  << "unsaved" << (hasUnsavedChanges() ? "yes" : "no") << "port" << updated.port;
    emit changed();
}

void XraySettingsModel::forgetRemovedPreset()
{
    XrayServerConfig &srv = m_protocolConfig.serverConfig;
    if (srv.presetId.isEmpty() || m_presetsModel->preset(srv.presetId)) {
        return;
    }
    const QString removedId = srv.presetId;
    srv.presetId = xrayPresets::matchBuiltin(srv.inboundTemplate, srv.site, srv.port);
    if (m_originalProtocolConfig.serverConfig.presetId == removedId) {
        m_originalProtocolConfig.serverConfig.presetId = srv.presetId;
    }
    m_presetsModel->setSelectedId(srv.presetId);
    logger.info() << "forgetRemovedPreset:" << removedId << "->"
                  << (srv.presetId.isEmpty() ? QStringLiteral("custom") : srv.presetId) << "unsaved"
                  << (hasUnsavedChanges() ? "yes" : "no");
    emit changed();
}

void XraySettingsModel::refreshDraftValidation()
{
    const XrayInboundValidation validation = validateXrayInbound(m_draft);
    const XrayInboundIssue *error = validation.firstError();
    m_draftValid = validation.isValid();
    m_draftErrorLine = error ? error->line : 0;
    if (error) {
        m_draftStatus = issueText(*error);
    } else {
        m_draftStatus = tr("JSON is valid. XRay core rules are met");
    }
    logger.info() << "draft: valid" << (m_draftValid ? "yes" : "no") << "errorLine" << QString::number(m_draftErrorLine)
                  << "modified" << (draftModified() ? "yes" : "no") << "length" << QString::number(m_draft.size())
                  << m_draftStatus;
    emit draftChanged();
}

#ifndef XRAYSETTINGSMODEL_H
#define XRAYSETTINGSMODEL_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/containerEnum.h"

class XrayPresetsModel;

class XraySettingsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString presetId READ presetId NOTIFY changed)
    Q_PROPERTY(QString presetName READ presetName NOTIFY changed)
    Q_PROPERTY(bool presetEdited READ presetEdited NOTIFY changed)
    Q_PROPERTY(int port READ port NOTIFY changed)
    Q_PROPERTY(QString inboundText READ inboundText NOTIFY changed)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY changed)
    Q_PROPERTY(bool canReset READ canReset NOTIFY changed)

    Q_PROPERTY(QString draftText READ draftText NOTIFY draftChanged)
    Q_PROPERTY(bool draftModified READ draftModified NOTIFY draftChanged)
    Q_PROPERTY(bool draftValid READ draftValid NOTIFY draftChanged)
    Q_PROPERTY(int draftErrorLine READ draftErrorLine NOTIFY draftChanged)
    Q_PROPERTY(QString draftStatus READ draftStatus NOTIFY draftChanged)
    Q_PROPERTY(bool draftHasError READ draftHasError NOTIFY draftChanged)

public:
    explicit XraySettingsModel(XrayPresetsModel *presetsModel, QObject *parent = nullptr);

    QString presetId() const;
    QString presetName() const;
    bool presetEdited() const;
    int port() const;
    QString inboundText() const;
    bool hasUnsavedChanges() const;
    bool canReset() const;

    QString draftText() const;
    bool draftModified() const;
    bool draftValid() const;
    int draftErrorLine() const;
    QString draftStatus() const;
    bool draftHasError() const;

    Q_INVOKABLE void selectPreset(const QString &id);
    Q_INVOKABLE void resetToPreset();
    Q_INVOKABLE void openEditor();
    Q_INVOKABLE void setDraft(const QString &text);
    Q_INVOKABLE bool formatDraft();
    Q_INVOKABLE bool commitDraft();
    Q_INVOKABLE QString saveAsPreset(const QString &name);
    Q_INVOKABLE QVariantMap validate(const QString &text) const;
    Q_INVOKABLE QStringList validationErrors() const;
    Q_INVOKABLE bool touchesServer() const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig &protocolConfig);
    amnezia::XrayProtocolConfig getProtocolConfig();

signals:
    void changed();
    void draftChanged();

private:
    void applyInbound(const QString &text);
    void refreshDraftValidation();
    void forgetRemovedPreset();

    XrayPresetsModel *m_presetsModel;
    amnezia::DockerContainer m_container = amnezia::DockerContainer::None;
    amnezia::XrayProtocolConfig m_protocolConfig;
    amnezia::XrayProtocolConfig m_originalProtocolConfig;

    QString m_draft;
    bool m_draftValid = false;
    int m_draftErrorLine = 0;
    QString m_draftStatus;
};

#endif // XRAYSETTINGSMODEL_H

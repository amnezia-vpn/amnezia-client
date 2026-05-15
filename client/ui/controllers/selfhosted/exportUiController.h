#ifndef EXPORTUICONTROLLER_H
#define EXPORTUICONTROLLER_H

#include <QObject>

#include "core/controllers/selfhosted/exportController.h"
#include "core/utils/errorCodes.h"

class ExportUiController : public QObject
{
    Q_OBJECT
public:
    explicit ExportUiController(ExportController* exportController, QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY exportConfigChanged)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY exportConfigChanged)
    Q_PROPERTY(QString config READ getConfig NOTIFY exportConfigChanged)
    Q_PROPERTY(QString nativeConfigString READ getNativeConfigString NOTIFY exportConfigChanged)

public slots:
    void generateFullAccessConfig(const QString &serverId);

    void generateConnectionConfig(const QString &serverId, int containerIndex, const QString &clientName);
    void generateOpenVpnConfig(const QString &serverId, const QString &clientName);
    void generateWireGuardConfig(const QString &serverId, const QString &clientName);
    void generateAwgConfig(const QString &serverId, int containerIndex, const QString &clientName);
    void generateXrayConfig(const QString &serverId, const QString &clientName);
    void generateQrFromString(const QString &text);

    QString getConfig();
    QString getNativeConfigString();
    QList<QString> getQrCodes();

    void exportConfig(const QString &fileName);

    void updateClientManagementModel(const QString &serverId, int containerIndex);

    void revokeConfig(int row, const QString &serverId, int containerIndex);

    void renameClient(int row, const QString &clientName, const QString &serverId, int containerIndex);

signals:
    void generateConfig(int type);
    void revokeConfigFinished();
    void exportErrorOccurred(const QString &errorMessage);
    void exportErrorOccurred(ErrorCode errorCode);

    void exportConfigChanged();

    void saveFile(const QString &fileName, const QString &data);

private:
    int getQrCodesCount();
    void clearPreviousConfig();
    void applyExportResult(const ExportController::ExportResult &result);

    ExportController* m_exportController;

    QString m_config;
    QString m_nativeConfigString;
    QList<QString> m_qrCodes;
};

#endif // EXPORTUICONTROLLER_H

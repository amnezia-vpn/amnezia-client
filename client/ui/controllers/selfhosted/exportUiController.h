#ifndef EXPORTUICONTROLLER_H
#define EXPORTUICONTROLLER_H

#include <QObject>

#include "core/controllers/selfhosted/exportController.h"

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
    void generateFullAccessConfig(int serverIndex);
    void generateConnectionConfig(int serverIndex, int containerIndex, const QString &clientName);
    void generateOpenVpnConfig(int serverIndex, const QString &clientName);
    void generateWireGuardConfig(int serverIndex, const QString &clientName);
    void generateAwgConfig(int serverIndex, int containerIndex, const QString &clientName);
    void generateXrayConfig(int serverIndex, const QString &clientName);

    QString getConfig();
    QString getNativeConfigString();
    QList<QString> getQrCodes();

    void exportConfig(const QString &fileName);

    void updateClientManagementModel(int serverIndex, int containerIndex);
    void revokeConfig(int row, int serverIndex, int containerIndex);
    void renameClient(int row, const QString &clientName, int serverIndex, int containerIndex);

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

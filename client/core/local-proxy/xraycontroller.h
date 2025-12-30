#pragma once

#include <QObject>
#include <QString>

class XrayController : public QObject {
    Q_OBJECT

public:
    explicit XrayController(QObject* parent = nullptr);
    ~XrayController();

    bool start(const QString& configPath);
    bool stop();
    bool isXrayRunning() const;
    qint64 getProcessId() const;
    QString getError() const;

private:
    bool loadConfigFile(const QString& configPath, QString& configContent);

    bool m_isRunning {false};
    QString m_lastError;
}; 
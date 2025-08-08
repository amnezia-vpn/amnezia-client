#pragma once

#include <QObject>
#include <QProcess>
#include <QScopedPointer>

class XrayController : public QObject {
    Q_OBJECT

public:
    explicit XrayController(QObject* parent = nullptr);
    ~XrayController();

    bool start(const QString& configPath);
    void stop();
    bool isXrayRunning() const;
    qint64 getProcessId() const;
    QString getError() const;

private:
    QString getXrayExecutablePath() const;
    QStringList getXrayArguments(const QString& configPath) const;

    QScopedPointer<QProcess> m_process;
}; 
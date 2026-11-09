#ifndef LOCALPROXYUICONTROLLER_H
#define LOCALPROXYUICONTROLLER_H

#include <QObject>

#include "core/controllers/localProxyController.h"

class LocalProxyUiController : public QObject
{
    Q_OBJECT
public:
    explicit LocalProxyUiController(LocalProxyController *localProxyController, QObject *parent = nullptr);

    Q_PROPERTY(bool isSupported READ isSupported CONSTANT)
    Q_PROPERTY(bool isEnabled READ isEnabled NOTIFY stateChanged)
    Q_PROPERTY(int port READ port NOTIFY stateChanged)
    Q_PROPERTY(QString ownerId READ ownerId NOTIFY stateChanged)
    Q_PROPERTY(int activePort READ activePort NOTIFY stateChanged)

public slots:
    bool isSupported() const;
    bool isEnabled() const;
    int port() const;
    QString ownerId() const;
    int activePort() const;
    bool isPortUserDefined() const;

    bool setPort(int port);
    bool isPortBusy(int port) const;
    bool enable(const QString &ownerId, int port);
    void disable();

signals:
    void stateChanged();
    void startFailed(const QString &message);

private:
    LocalProxyController *m_localProxyController;
};

#endif // LOCALPROXYUICONTROLLER_H

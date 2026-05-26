#ifndef TUNNEL_H
#define TUNNEL_H

#include <QJsonObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "core/protocols/vpnProtocol.h"
#include "core/utils/containerEnum.h"
#include "core/utils/errorCodes.h"

class QTimer;

class Tunnel : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Preparing,
        Prepared,
        Committing,
        Active,
        Gone,
        Failed,
    };
    Q_ENUM(State)

    Tunnel(QString ifname,
           amnezia::DockerContainer container,
           QJsonObject config,
           QString remoteAddress,
           QObject* parent = nullptr);
    ~Tunnel() override;

    const QString& ifname() const { return m_ifname; }
    const QString& remoteAddress() const { return m_remoteAddress; }
    amnezia::DockerContainer container() const { return m_container; }
    const QJsonObject& config() const { return m_config; }
    State state() const { return m_state; }
    QSharedPointer<VpnProtocol> protocol() const { return m_protocol; }

    const QString& handoverIfname() const { return m_handoverIfname; }
    void setHandoverIfname(const QString& ifname) { m_handoverIfname = ifname; }
    void clearHandoverIfname() { m_handoverIfname.clear(); }

    virtual void prepare();
    virtual void commit();
    virtual void deactivate();
    virtual void restart();

signals:
    void prepared();
    void activated();
    void failed(amnezia::ErrorCode);
    void stateChanged(Tunnel::State);
    void bytesChanged(quint64 rxBytes, quint64 txBytes);
    void addressesUpdated(const QString& gateway, const QString& localAddress);

protected:
    void setState(State);
    void startActivationDeadline(int msec);
    void cancelActivationDeadline();

    QString m_ifname;
    QString m_remoteAddress;
    QString m_handoverIfname;
    amnezia::DockerContainer m_container;
    QJsonObject m_config;
    QSharedPointer<VpnProtocol> m_protocol;

private:
    void onProtocolStateChanged(Vpn::ConnectionState state);
    void onPrimaryReady();
    void onPrimaryFailed();

    State m_state = State::Idle;
    QTimer* m_deadline = nullptr;
};

#endif  // TUNNEL_H

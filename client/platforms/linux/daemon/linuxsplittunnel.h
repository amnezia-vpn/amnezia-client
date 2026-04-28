#ifndef LINUXSPLITTUNNEL_H
#define LINUXSPLITTUNNEL_H

#include <QObject>
#include <QStringList>
#include <QTimer>

class LinuxSplitTunnel : public QObject {
    Q_OBJECT
public:
    explicit LinuxSplitTunnel(QObject* parent = nullptr);
    ~LinuxSplitTunnel();

    void start(const QString& physicalGateway, const QString& physicalInterface);
    void stop();
    void setExcludedApps(const QStringList& appPaths);
    bool isRunning() const { return m_running; }

    // Finds the physical (non-VPN) gateway and interface used to reach vpnServerIp.
    // Call before the VPN default route replaces the physical one, or use the
    // pinned /32 route to the VPN server which always stays on the physical interface.
    static bool findPhysicalRoute(const QString& vpnServerIp, QString& gateway, QString& iface);

private:
    void scanAndAssignProcesses();
    void assignPid(qint64 pid);
    void setupNetworking();
    void teardownNetworking();
    void setupBypassRoute();
    void teardownBypassRoute();

    QStringList m_excludedAppPaths;
    QString m_physicalGateway;
    QString m_physicalInterface;
    QTimer* m_timer = nullptr;
    bool m_running = false;
};

#endif // LINUXSPLITTUNNEL_H

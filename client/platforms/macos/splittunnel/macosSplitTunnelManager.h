#ifndef MACOSSPLITTUNNELMANAGER_H
#define MACOSSPLITTUNNELMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>

#include "core/utils/commonStructs.h"
#include "core/utils/routeModes.h"

/*!
 * Drives the macOS split-tunnel system extension:
 *   - registers/unregisters the extension,
 *   - owns the NETransparentProxyManager configuration,
 *   - starts/stops the transparent proxy and pushes the app list into it.
 *
 * Only used by the Developer ID (.pkg) build; the App Store build (MACOS_NE)
 * never compiles this file.
 *
 * Threading: the public methods must be called from the Qt main thread.
 * NetworkExtension completion handlers can run on arbitrary queues, so every
 * signal is re-posted to this object's thread before being emitted.
 *
 * State machine. reconcile() is the only entry point that decides anything; it
 * is idempotent, so callers may fire it on every connection state change:
 *
 *   desired (shouldRun) x actual (m_proxyStatus) -> action
 *     true,  not running        -> activate extension (once), then startProxy
 *     true,  running, same opts -> nothing
 *     true,  running, new opts  -> sendProviderMessage
 *     false, running            -> stopProxy
 *     false, not running        -> nothing
 */
class MacOSSplitTunnelManager : public QObject
{
    Q_OBJECT

public:
    static MacOSSplitTunnelManager *instance();

    /*! Submits an activation request if one has not been submitted yet. */
    void activateExtension();

    /*! Brings the proxy in line with the desired state. Cheap and idempotent. */
    void reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                   const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);

    /*! Stops the proxy, removes the saved NE configuration and unregisters the
     *  extension - in that order, each step waiting for the previous one. Call
     *  when the user switches app split tunneling off. */
    void disableFeature();

signals:
    /*! macOS is waiting for the user to approve the extension in
     *  System Settings -> General -> Login Items & Extensions. */
    void needsUserApproval();
    /*! The extension was approved and registered. */
    void extensionActivated();
    /*! The transparent proxy actually reached the running state. */
    void proxyStarted();
    void proxyStopped();
    void errorOccurred(const QString &message);

private:
    explicit MacOSSplitTunnelManager(QObject *parent = nullptr);

    struct DesiredState
    {
        bool valid = false;
        bool shouldRun = false;
        QVector<amnezia::InstalledAppInfo> apps;
        QString vpnServer;
    };

    void startProxy();
    void stopProxy();
    void removeConfiguration(void (^completion)(void));
    void deactivateExtension();
    QByteArray optionsJson(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer) const;

    /*! Emits a signal on this object's thread, whatever queue we are on. */
    void postError(const QString &message);
    void postNeedsApproval();
    void postActivated();

    void onExtensionActivated();
    /*! Called from the NEVPNStatusDidChangeNotification observer. */
    void onProxyStatusChanged(int status);

    bool m_activationRequested = false;
    bool m_extensionActivated = false;
    /*! Mirrors NEVPNStatus; -1 until the configuration has been loaded once. */
    int m_proxyStatus = -1;
    bool m_tearingDown = false;
    /*! Logged once per session instead of on every reconcile. */
    bool m_warnedAboutEmptyServer = false;

    DesiredState m_desired;
    QByteArray m_appliedOptions;
};

#endif

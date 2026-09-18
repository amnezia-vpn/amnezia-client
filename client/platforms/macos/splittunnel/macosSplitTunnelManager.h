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

    /*! Submits an activation request. Pass userInitiated when the user just
     *  switched the feature on: a pending approval that the user never answered
     *  is then cleared first, so macOS offers the install prompt again instead
     *  of silently sitting on the old request. */
    void activateExtension(bool userInitiated = false);

    /*! Brings the proxy in line with the desired state. Cheap and idempotent. */
    void reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                   const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);

    /*! Switches the feature off without uninstalling anything: the proxy is
     *  stopped and the NE configuration is disabled, so the extension keeps its
     *  row in System Settings -> Network Extensions with the toggle off and the
     *  user is never asked to approve it again. Call when the user switches app
     *  split tunneling off. */
    void disableFeature();

    /*! Removes every trace: stops the proxy, deletes the saved NE configuration
     *  and unregisters the system extension. Only for an actual uninstall - a
     *  plain off/on cycle must use disableFeature(). */
    void uninstallFeature();

    /*! Asks the system for the extension's real state and emits
     *  extensionStateChanged. Cheap; safe to call on startup and whenever the
     *  UI needs to show what System Settings shows. */
    void refreshExtensionState();

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
    /*! What the system reports about the extension. "installed" means macOS has
     *  it registered, "enabled" is the toggle the user sees in System Settings
     *  -> General -> Login Items & Extensions -> Network Extensions. */
    void extensionStateChanged(bool installed, bool enabled, bool awaitingApproval);

private:
    explicit MacOSSplitTunnelManager(QObject *parent = nullptr);

    struct DesiredState
    {
        bool valid = false;
        bool shouldRun = false;
        amnezia::AppsRouteMode mode = amnezia::AppsRouteMode::VpnAllExceptApps;
        QVector<amnezia::InstalledAppInfo> apps;
        QString vpnServer;
    };

    void startProxy();
    void stopProxy();
    /*! Disables the saved NE configuration, keeping it in System Settings. */
    void setConfigurationEnabled(bool enabled, void (^completion)(void));
    void removeConfiguration(void (^completion)(void));
    /*! Unregisters the extension - the API equivalent of
     *  "systemextensionsctl uninstall <team> <bundle id>". With
     *  reactivateAfterwards the activation request is submitted again once the
     *  removal completes, which is what clears a stuck approval. */
    void deactivateExtension(bool reactivateAfterwards = false);
    void onExtensionStateKnown(bool installed, bool enabled, bool awaitingApproval);
    QByteArray optionsJson(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer,
                           amnezia::AppsRouteMode mode) const;

    /*! Emits a signal on this object's thread, whatever queue we are on. */
    void postError(const QString &message);
    void postNeedsApproval();
    void postActivated();

    void onExtensionActivated();
    /*! Called from the NEVPNStatusDidChangeNotification observer. */
    void onProxyStatusChanged(int status);

    /*! True only while an activation request is in flight, so a second one is
     *  not submitted on top of it. Cleared when the request finishes either way
     *  - the lasting state lives in m_extensionInstalled/m_extensionEnabled. */
    bool m_activationRequested = false;
    bool m_extensionActivated = false;
    /*! Last state reported by the system extension properties request. */
    bool m_extensionInstalled = false;
    bool m_extensionEnabled = false;
    bool m_extensionAwaitingApproval = false;
    /*! Mirrors NEVPNStatus; -1 until the configuration has been loaded once. */
    int m_proxyStatus = -1;
    bool m_tearingDown = false;
    /*! Logged once per session instead of on every reconcile. */
    bool m_warnedAboutEmptyServer = false;

    DesiredState m_desired;
    QByteArray m_appliedOptions;
};

#endif

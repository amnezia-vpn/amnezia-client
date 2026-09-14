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
 *   - registers/unregisters AmneziaVPNSplitTunnel.systemextension,
 *   - owns the NETransparentProxyManager configuration,
 *   - starts/stops the transparent proxy and pushes the app list into it.
 *
 * Only used by the Developer ID (.pkg) build; the App Store build (MACOS_NE)
 * never compiles this file.
 *
 * Threading: the public methods must be called from the Qt main thread.
 * NetworkExtension completion handlers can run on arbitrary queues, so every
 * signal is re-posted to this object's thread before being emitted.
 */
class MacOSSplitTunnelManager : public QObject
{
    Q_OBJECT

public:
    static MacOSSplitTunnelManager *instance();

    /*! Submits an activation request if one has not been submitted yet. */
    void activateExtension();

    /*! Brings the proxy in line with the desired state. Safe to call often. */
    void reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                   const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);

    /*! Stops the proxy, removes the saved NE configuration and unregisters the
     *  system extension. Call when the user switches app split tunneling off,
     *  so nothing is left behind in System Settings. */
    void disableFeature();

    void stopProxy();

signals:
    /*! macOS is waiting for the user to approve the extension in
     *  System Settings -> General -> Login Items & Extensions. */
    void needsUserApproval();
    /*! The extension was approved and registered. */
    void extensionActivated();
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

    void startProxy(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);
    void removeConfiguration();
    void deactivateExtension();
    QByteArray optionsJson(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer) const;

    /*! Emits a signal on this object's thread, whatever queue we are on. */
    void postError(const QString &message);
    void postNeedsApproval();
    void postActivated();

    /*! Called from the OSSystemExtensionRequest delegate once the extension is
     *  registered, so a first-run activation actually starts the proxy. */
    void onExtensionActivated();

    bool m_activationRequested = false;
    bool m_extensionActivated = false;
    DesiredState m_desired;
    QByteArray m_appliedOptions;
};

#endif

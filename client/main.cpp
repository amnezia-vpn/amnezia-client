#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "amneziaApplication.h"
#include "core/utils/osSignalHandler.h"
#include "core/utils/migrations.h"
#include "version.h"

#include <QLocalSocket>

#ifdef Q_OS_WIN
    #include "Windows.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/QtAppDelegate-C-Interface.h"
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
namespace {
QString findVpnDeepLinkInArguments(const QStringList &args)
{
    for (const QString &arg : args) {
        const QString t = arg.trimmed();
        if (t.startsWith(QLatin1String("vpn://"), Qt::CaseInsensitive)) {
            return t;
        }
    }
    return {};
}

bool notifyRunningInstanceOrExit(AmneziaApplication &app, const QString &vpnPayload)
{
    QLocalSocket socket;
    socket.connectToServer(QStringLiteral("AmneziaVPNInstance"));
    if (!socket.waitForConnected(500)) {
        return false;
    }
    qWarning() << "AmneziaVPN is already running";
    if (!vpnPayload.isEmpty()) {
        const QByteArray msg = QByteArrayLiteral("VPN\n") + vpnPayload.toUtf8() + '\n';
        socket.write(msg);
        socket.waitForBytesWritten(3000);
    }
    socket.flush();
    QTimer::singleShot(1000, &app, [&app]() { app.quit(); });
    return true;
}
} // namespace
#endif

// Desktop (non-NE): single-instance IPC forwards vpn:// to the running process. MACOS_NE has no IPC here;
// deep links use argv / QFileOpenEvent after registration in the app bundle Info.plist.

int main(int argc, char *argv[])
{
    Migrations migrationsManager;
    migrationsManager.doMigrations();

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

#ifdef Q_OS_ANDROID
    // QTBUG-95974 QTBUG-95764 QTBUG-102168
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qputenv("ANDROID_OPENSSL_SUFFIX", "_3");
#endif

    AmneziaApplication app(argc, argv);
    OsSignalHandler::setup();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    const QString vpnFromArgv = findVpnDeepLinkInArguments(QCoreApplication::arguments());
    if (notifyRunningInstanceOrExit(app, vpnFromArgv)) {
        return app.exec();
    }
    app.startLocalServer();
#endif

// Allow to raise app window if secondary instance launched
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    app.registerTypes();

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    app.loadFonts();

    bool doExec = app.parseCommands();

    if (doExec) {
        app.init();

        qInfo().noquote() << QString("Started %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());

        return app.exec();
    }
    return 0;
}

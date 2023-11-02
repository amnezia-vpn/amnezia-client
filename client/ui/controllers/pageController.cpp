#include "pageController.h"

#include <QProcess>

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#ifdef Q_OS_ANDROID
    #include "../../platforms/android/androidutils.h"
    #include <QJniObject>
#endif
#if defined Q_OS_MAC
    #include "ui/macos_util.h"
#endif

PageController::PageController(const QSharedPointer<ServersModel> &serversModel,
                               const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_settings(settings)
{
#ifdef Q_OS_ANDROID
    // Change color of navigation and status bar's
    auto initialPageNavigationBarColor = getInitialPageNavigationBarColor();
    AndroidUtils::runOnAndroidThreadSync([&initialPageNavigationBarColor]() {
        QJniObject activity = AndroidUtils::getActivity();
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid()) {
            window.callMethod<void>("addFlags", "(I)V", 0x80000000);
            window.callMethod<void>("clearFlags", "(I)V", 0x04000000);
            window.callMethod<void>("setStatusBarColor", "(I)V", 0xFF0E0E11);
            window.callMethod<void>("setNavigationBarColor", "(I)V", initialPageNavigationBarColor);
        }
    });
#endif

#if defined Q_OS_MACX
    connect(this, &PageController::raiseMainWindow, []() { setDockIconVisible(true); });
    connect(this, &PageController::hideMainWindow, []() { setDockIconVisible(false); });
#endif
    
    m_isTriggeredByConnectButton = false;
}

QString PageController::getInitialPage()
{
    if (m_serversModel->getServersCount()) {
        if (m_serversModel->getDefaultServerIndex() < 0) {
            auto defaultServerIndex = m_serversModel->index(0);
            m_serversModel->setData(defaultServerIndex, true, ServersModel::Roles::IsDefaultRole);
        }
        return getPagePath(PageLoader::PageEnum::PageStart);
    } else {
        return getPagePath(PageLoader::PageEnum::PageSetupWizardStart);
    }
}

QString PageController::getPagePath(PageLoader::PageEnum page)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PageLoader::PageEnum>();
    QString pageName = metaEnum.valueToKey(static_cast<int>(page));
    return "qrc:/ui/qml/Pages2/" + pageName + ".qml";
}

void PageController::closeWindow()
{
#ifdef Q_OS_ANDROID
    qApp->quit();
#else
    if (m_serversModel->getServersCount() == 0) {
        qApp->quit();
    } else {
        emit hideMainWindow();
    }
#endif
}

void PageController::keyPressEvent(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Back: emit closePage();
    default: return;
    }
}

unsigned int PageController::getInitialPageNavigationBarColor()
{
    if (m_serversModel->getServersCount()) {
        return 0xFF1C1D21;
    } else {
        return 0xFF0E0E11;
    }
}

void PageController::updateNavigationBarColor(const int color)
{
#ifdef Q_OS_ANDROID
    // Change color of navigation bar
    AndroidUtils::runOnAndroidThreadSync([&color]() {
        QJniObject activity = AndroidUtils::getActivity();
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid()) {
            window.callMethod<void>("setNavigationBarColor", "(I)V", color);
        }
    });
#endif
}

void PageController::showOnStartup()
{
    if (!m_settings->isStartMinimized()) {
        emit raiseMainWindow();
    } else {
#ifdef Q_OS_WIN
        emit hideMainWindow();
#elif defined Q_OS_MACX
        setDockIconVisible(false);
#endif
    }
}

void PageController::updateDrawerRootPage(PageLoader::PageEnum page)
{
    m_drawerLayer = 0;
    m_currentRootPage = page;
}

void PageController::goToDrawerRootPage()
{

    m_drawerLayer = 0;

    emit showTopCloseButton(false);
    emit forceCloseDrawer();
}

void PageController::drawerOpen()
{
    m_drawerLayer = m_drawerLayer + 1;
    emit showTopCloseButton(true);
}

void PageController::drawerClose()
{
    m_drawerLayer = m_drawerLayer -1;
    if (m_drawerLayer <= 0) {
        emit showTopCloseButton(false);
        m_drawerLayer = 0;
    }
}

bool PageController::isTriggeredByConnectButton()
{
    return m_isTriggeredByConnectButton;
}

void PageController::setTriggeredBtConnectButton(bool trigger)
{
    m_isTriggeredByConnectButton = trigger;
}

void PageController::closeApplication()
{
    qApp->quit();
}

bool PageController::checkForUpdates()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return false;
#else
    QString path = qApp->applicationDirPath();

    bool checked = false;

#ifdef Q_OS_MACOS
    if(path.contains(qApp->applicationName()+".app/Contents/MacOS")) {
        path = path.remove("Contents/MacOS");
    }
    path = path + "maintenancetool.app";

    checked = true;
#endif

#ifdef Q_OS_LINUX
    if(path.contains("/client/bin")) {
        path = path.remove("/client/bin");
    }
    path = path + "/maintenancetool";

    checked = true;
#endif

    if (!checked) {
        return false;
    }

    QStringList argsCheckUpdates;
    argsCheckUpdates << "--checkupdates";

    QProcess process;
    process.start(path, argsCheckUpdates);

    // Wait until the update tool is finished
    process.waitForFinished();

    if (process.error() != QProcess::UnknownError) {
        emit showNotificationMessage(tr("Checking for updates: %1").arg(process.errorString()));
        return true;
    }

    // Read the output
    QByteArray data = process.readAllStandardOutput();

    // No output means no updates available
    // Note that the exit code will also be 1, but we don't use that
    // Also note that we should parse the output instead of just checking if it is empty if we want specific update info
    if (data.isEmpty()) {
        emit showNotificationMessage(tr("Checking for updates: %1").arg("it's the latest version"));
        return true;
    }

    // Note: we start it detached because this application need to close for the update
    QStringList argsUpdater("--updater");
    bool bresult = QProcess::startDetached(path, argsUpdater);
    if (!bresult) {
        emit showNotificationMessage(tr("Checking for updates: %1").arg("test"));
        return true;
    }


    // Close the application
    qApp->quit();
    return true;
#endif
}

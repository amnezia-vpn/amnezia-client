#include "pageController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_utils.h"
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
    case Qt::Key_Back:
    case Qt::Key_Escape: {
        if (m_drawerDepth) {
            emit closeTopDrawer();
            setDrawerDepth(getDrawerDepth() - 1);
        } else {
            emit escapePressed();
        }
        break;
    }
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

bool PageController::isTriggeredByConnectButton()
{
    return m_isTriggeredByConnectButton;
}

void PageController::setTriggeredByConnectButton(bool trigger)
{
    m_isTriggeredByConnectButton = trigger;
}

void PageController::closeApplication()
{
    qApp->quit();
}

void PageController::setDrawerDepth(const int depth)
{
    if (depth >= 0) {
        m_drawerDepth = depth;
    }
}

int PageController::getDrawerDepth()
{
    return m_drawerDepth;
}

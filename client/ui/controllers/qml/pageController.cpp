#include "pageController.h"

#include "ui/utils/converter.h"
#include "core/utils/errorStrings.h"
#include "core/utils/appUiConfig.h"
#if defined(MACOS_NE)
#include "platforms/ios/ios_controller.h"
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
    #include <QInputMethod>
#else
    #include <QApplication>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif
#if defined Q_OS_MAC
    #include "ui/utils/macosUtil.h"
#endif

PageController::PageController(ServersController* serversController, SettingsController* settingsController,
                               QObject *parent)
    : QObject(parent), m_serversController(serversController), m_settingsController(settingsController)
{
#ifdef Q_OS_ANDROID
    auto initialPageNavigationBarColor = getInitialPageNavigationBarColor();
    AndroidController::instance()->setNavigationBarColor(initialPageNavigationBarColor);

    connect(AndroidController::instance(), &AndroidController::imeInsetsChanged, this, [this](int heightDp) {
        m_imeHeight = heightDp;
        emit imeHeightChanged(heightDp);
        emit safeAreaBottomMarginChanged();
    });
    connect(AndroidController::instance(), &AndroidController::systemBarsInsetsChanged, this, [this](int navBarHeightDp, int statusBarHeightDp) {
        m_cachedNavigationBarHeight = navBarHeightDp;
        m_cachedStatusBarHeight = statusBarHeightDp;
        emit safeAreaBottomMarginChanged();
        emit safeAreaTopMarginChanged();
    });
#endif

#ifdef Q_OS_IOS
    connect(QGuiApplication::inputMethod(), &QInputMethod::visibleChanged, this, &PageController::safeAreaBottomMarginChanged);
#endif

#if defined Q_OS_MACX
    connect(this, &PageController::raiseMainWindow, []() {
        setDockIconVisible(true);
    });
    connect(this, &PageController::hideMainWindow, []() {
        setDockIconVisible(false);
    });
#endif

    connect(this, qOverload<ErrorCode>(&PageController::showErrorMessage), this, &PageController::onShowErrorMessage);
    
    m_isTriggeredByConnectButton = false;
}

bool PageController::isStartPageVisible()
{
    return m_serversController->getServersCount() == 0;
}

QString PageController::getPagePath(PageLoader::PageEnum page)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PageLoader::PageEnum>();
    QString pageName = metaEnum.valueToKey(static_cast<int>(page));

    return QStringLiteral(APP_QML_PAGES_PREFIX) + pageName + QStringLiteral(".qml");
}

void PageController::closeWindow()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    qApp->quit();
#else
    emit hideMainWindow();
#endif
}

void PageController::hideWindow()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->minimizeApp();
#endif
}

unsigned int PageController::getInitialPageNavigationBarColor()
{
    if (m_serversController->getServersCount()) {
        return 0xFF1F1F23;
    } else {
        return 0xFF101012;
    }
}

void PageController::updateNavigationBarColor(const int color)
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->setNavigationBarColor(color);
#endif
}

void PageController::showOnStartup()
{
    if (!m_settingsController->isStartMinimizedEnabled()) {
        emit raiseMainWindow();
    } else {
#if defined(Q_OS_WIN) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
        emit hideMainWindow();
#elif defined(Q_OS_MACX)
        setDockIconVisible(false);
#endif
    }
}

bool PageController::shouldStartMinimized() const
{
    return m_settingsController->isStartMinimizedEnabled();
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

bool PageController::isEdgeToEdgeEnabled()
{
#ifdef Q_OS_ANDROID
    if (!m_edgeToEdgeCached) {
        m_cachedEdgeToEdgeEnabled = AndroidController::instance()->isEdgeToEdgeEnabled();
        m_edgeToEdgeCached = true;
    }
    return m_cachedEdgeToEdgeEnabled;
#else
    return false;
#endif
}

int PageController::getStatusBarHeight()
{
#ifdef Q_OS_ANDROID
    if (m_cachedStatusBarHeight < 0) {
        m_cachedStatusBarHeight = AndroidController::instance()->getStatusBarHeight();
    }
    return m_cachedStatusBarHeight;
#else
    return 0;
#endif
}

int PageController::getNavigationBarHeight()
{
#ifdef Q_OS_ANDROID
    if (m_cachedNavigationBarHeight < 0) {
        m_cachedNavigationBarHeight = AndroidController::instance()->getNavigationBarHeight();
    }
    return m_cachedNavigationBarHeight;
#else
    return 0;
#endif
}

int PageController::getSafeAreaTopMargin()
{
#ifdef Q_OS_ANDROID
    if (isEdgeToEdgeEnabled()) {
        int height = getStatusBarHeight();
        int result = height > 0 ? height : 40;
        return result;
    }
#endif
    return m_safeAreaMargins.top();
}

int PageController::getSafeAreaBottomMargin()
{
#ifdef Q_OS_ANDROID
    if (isEdgeToEdgeEnabled()) {
        if (m_imeHeight > 0) {
            return 0;
        }
        
        int height = getNavigationBarHeight();
        int result = height > 0 ? height : 56;
        return result;
    }
#endif
#ifdef Q_OS_IOS
    if (QGuiApplication::inputMethod()->isVisible()) {
        return 0;
    }
#endif
    return m_safeAreaMargins.bottom();
}

void PageController::setSafeAreaMargins(const QMargins &margins)
{
    const QMargins previous = m_safeAreaMargins;
    m_safeAreaMargins = margins;
    if (previous.top() != margins.top()) {
        emit safeAreaTopMarginChanged();
    }
    if (previous.bottom() != margins.bottom()) {
        emit safeAreaBottomMarginChanged();
    }
}

int PageController::getImeHeight()
{
    return m_imeHeight;
}

void PageController::onShowErrorMessage(ErrorCode errorCode)
{
    const auto fullErrorMessage = errorString(errorCode);
    const auto errorMessage = fullErrorMessage.mid(fullErrorMessage.indexOf(". ") + 1); // remove ErrorCode %1.
    const auto errorUrl = QStringLiteral("troubleshooting/error-codes/#error-%1-%2").arg(static_cast<int>(errorCode)).arg(utils::enumToString(errorCode).toLower());
    const auto fullMessage = QStringLiteral("<a href=\"%1\">ErrorCode: %2</a>. %3").arg(errorUrl).arg(static_cast<int>(errorCode)).arg(errorMessage);

    emit showErrorMessage(fullMessage);
}

#include "pageController.h"
#include "utils/converter.h"
#include "core/errorstrings.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif
#if defined Q_OS_MAC
    #include "ui/macos_util.h"
#endif

PageController::PageController(const QSharedPointer<ServersModel> &serversModel,
                               const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_settings(settings)
{
#ifdef Q_OS_ANDROID
    auto initialPageNavigationBarColor = getInitialPageNavigationBarColor();
    AndroidController::instance()->setNavigationBarColor(initialPageNavigationBarColor);
#endif

#if defined Q_OS_MACX
    connect(this, &PageController::raiseMainWindow, []() { setDockIconVisible(true); });
    connect(this, &PageController::hideMainWindow, []() { setDockIconVisible(false); });
#endif

    connect(this, qOverload<ErrorCode>(&PageController::showErrorMessage), this, &PageController::onShowErrorMessage);
    
    m_isTriggeredByConnectButton = false;
}

bool PageController::isStartPageVisible()
{
    if (m_serversModel->getServersCount()) {
        if (m_serversModel->getDefaultServerIndex() < 0) {
            auto defaultServerIndex = m_serversModel->index(0);
            m_serversModel->setData(defaultServerIndex, true, ServersModel::Roles::IsDefaultRole);
        }
        return false;
    } else {
        return true;
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

void PageController::hideWindow()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->minimizeApp();
#endif
}

void PageController::keyPressEvent(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Back:
    case Qt::Key_Escape: {
        if (m_drawerDepth) {
            emit closeTopDrawer();
            decrementDrawerDepth();
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
    AndroidController::instance()->setNavigationBarColor(color);
#endif
}

void PageController::showOnStartup()
{
    if (!m_settings->isStartMinimized()) {
        emit raiseMainWindow();
    } else {
#if defined(Q_OS_WIN) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
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

int PageController::getDrawerDepth() const
{
    return m_drawerDepth;
}

int PageController::incrementDrawerDepth()
{
    return ++m_drawerDepth;
}

int PageController::decrementDrawerDepth()
{
    if (m_drawerDepth == 0) {
        return m_drawerDepth;
    } else {
        return --m_drawerDepth;
    }
}

void PageController::onShowErrorMessage(ErrorCode errorCode)
{
    const auto fullErrorMessage = errorString(errorCode);
    const auto errorMessage = fullErrorMessage.mid(fullErrorMessage.indexOf(". ") + 1); // remove ErrorCode %1.
    const auto errorUrl = QStringLiteral("https://docs.amnezia.org/troubleshooting/error-codes/#error-%1-%2").arg(static_cast<int>(errorCode)).arg(utils::enumToString(errorCode).toLower());
    const auto fullMessage = QStringLiteral("<a href=\"%1\" style=\"color: #FBB26A;\">ErrorCode: %2</a>. %3").arg(errorUrl).arg(static_cast<int>(errorCode)).arg(errorMessage);

    emit showErrorMessage(fullMessage);
}

#include "pagesUiTestFixture.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QJSValue>
#include <QObject>
#include <QQuickStyle>
#include <QSet>
#include <QUuid>
#include <QVariant>

#include <vpnConnection.h>

#include "ui/utils/pageEnum.h"

bool PagesUiTestFixture::initialize(QObject *parent)
{
    m_settings = new SecureQSettings("AmneziaVPN-PagesUiTest-" + QUuid::createUuid().toString(), "amnezia-client",
                                     parent, false);

    m_engine = new QQmlApplicationEngine(parent);
    m_engine->addImportPath(QStringLiteral("qrc:/ui/qml/Modules/"));
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", false);
    m_engine->rootContext()->setContextProperty("IsPlayBuild", false);

    auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

    m_core = new TestCoreController(vpnConnection, m_settings, m_engine, parent);

    m_pageController = m_core->pageController();

    return m_pageController != nullptr;
}

void PagesUiTestFixture::cleanup()
{
    closePage();

    if (m_settings) {
        m_settings->clearSettings();
    }

    delete m_core;
    m_core = nullptr;
    m_pageController = nullptr;

    delete m_engine;
    m_engine = nullptr;

    delete m_settings;
    m_settings = nullptr;
}

bool PagesUiTestFixture::loadPage(const QString &pageName)
{
    closePage();

    const QByteArray hostQml = QStringLiteral("import QtQuick\n"
                                              "import QtQuick.Controls\n"
                                              "import \"../Config\"\n"
                                              "\n"
                                              "ApplicationWindow {\n"
                                              "    width: 420\n"
                                              "    height: 800\n"
                                              "    visible: true\n"
                                              "    property var dialogs: Dialogs\n"
                                              "\n"
                                              "    Loader {\n"
                                              "        objectName: \"pageLoader\"\n"
                                              "        anchors.fill: parent\n"
                                              "        source: \"qrc:/ui/qml/Pages2/%1.qml\"\n"
                                              "    }\n"
                                              "}\n")
                                       .arg(pageName)
                                       .toUtf8();

    m_engine->loadData(hostQml, QUrl(QStringLiteral("qrc:/ui/qml/Pages2/pagesUiTestHost.qml")));

    if (m_engine->rootObjects().isEmpty()) {
        return false;
    }

    m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().last());

    if (!m_window || !QTest::qWaitForWindowExposed(m_window)) {
        return false;
    }

    m_window->requestActivate();

    if (!QTest::qWaitForWindowActive(m_window)) {
        return false;
    }

    QObject *loader = m_window->findChild<QObject *>(QStringLiteral("pageLoader"));

    if (!loader) {
        return false;
    }

    if (!QTest::qWaitFor([loader] { return loader->property("status").toInt() == 1; }, 5000)) {
        return false;
    }

    m_page = qvariant_cast<QQuickItem *>(loader->property("item"));

    return m_page != nullptr;
}

bool PagesUiTestFixture::loadPageFromStartPage(PageLoader::PageEnum page)
{
    if (!m_pageController || !m_pageController->isStartPageVisible()) {
        return false;
    }

    // Load the actual application start page
    if (!loadPage(QStringLiteral("PageStart"))) {
        return false;
    }

    // Navigate through PageController
    emit m_pageController->goToPage(page, false);

    const QString pagePath = m_pageController->getPagePath(page);
    QQuickItem *targetPage = nullptr;

    const bool pageLoaded = QTest::qWaitFor(
            [this, &targetPage, &pagePath]() {
                targetPage = item(QStringLiteral("objectName"), pagePath, m_page);

                return targetPage && targetPage->isVisible();
            },
            5000);

    if (!pageLoaded) {
        return false;
    }

    m_page = targetPage;
    return true;
}

void PagesUiTestFixture::closePage()
{
    m_page = nullptr;

    if (!m_window) {
        return;
    }

    m_window->close();
    m_window->deleteLater();
    m_window = nullptr;

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

static void collectFromObject(QObject *object, QList<QQuickItem *> &result, QSet<QObject *> &seen)
{
    if (!object || seen.contains(object)) {
        return;
    }

    seen.insert(object);

    if (auto *item = qobject_cast<QQuickItem *>(object)) {
        result.append(item);

        for (QQuickItem *child : item->childItems()) {
            collectFromObject(child, result, seen);
        }
    }

    const QObjectList objectChildren = object->children();
    for (QObject *child : objectChildren) {
        collectFromObject(child, result, seen);
    }
}

void PagesUiTestFixture::collectItems(QQuickItem *root, QList<QQuickItem *> &result)
{
    QSet<QObject *> seen;
    collectFromObject(root, result, seen);
}

QList<QQuickItem *> PagesUiTestFixture::items(QQuickItem *root) const
{
    QList<QQuickItem *> result;
    QSet<QObject *> seen;

    if (root) {
        collectFromObject(root, result, seen);
        return result;
    }

    collectFromObject(m_page, result, seen);

    if (m_window && m_window->contentItem()) {
        collectFromObject(m_window->contentItem(), result, seen);
    }

    return result;
}

static bool isShown(QQuickItem *item)
{
    if (!item || !item->isVisible()) {
        return false;
    }

    for (QObject *object = item; object; object = object->parent()) {
        if (object->inherits("QQuickPopup")) {
            return object->property("visible").toBool();
        }
    }

    return true;
}

bool PagesUiTestFixture::isQmlType(const QObject *object, const QString &qmlType)
{
    if (!object) {
        return false;
    }

    const QString className = QString::fromLatin1(object->metaObject()->className());
    return className == qmlType || className.startsWith(qmlType + QLatin1Char('_'));
}

QList<QQuickItem *> PagesUiTestFixture::itemsOfType(const QString &qmlType, QQuickItem *root) const
{
    QList<QQuickItem *> result;

    for (QQuickItem *candidate : items(root)) {
        if (isQmlType(candidate, qmlType)) {
            result.append(candidate);
        }
    }

    return result;
}

QQuickItem *PagesUiTestFixture::item(const QString &property, const QVariant &value, QQuickItem *root) const
{
    for (QQuickItem *candidate : items(root)) {
        const QVariant candidateValue = candidate->property(property.toUtf8().constData());

        if (candidateValue.isValid() && candidateValue == value) {
            return candidate;
        }
    }

    return nullptr;
}

QQuickItem *PagesUiTestFixture::visibleItem(const QString &property, const QVariant &value, QQuickItem *root) const
{
    for (QQuickItem *candidate : items(root)) {
        const QVariant candidateValue = candidate->property(property.toUtf8().constData());

        if (isShown(candidate) && candidateValue.isValid() && candidateValue == value) {
            return candidate;
        }
    }

    return nullptr;
}

QQuickItem *PagesUiTestFixture::clickable(const QString &text) const
{
    for (QQuickItem *candidate : items()) {
        const QVariant candidateText = candidate->property("text");

        if (candidateText.isValid() && candidateText.toString() == text && isShown(candidate)
            && candidate->isEnabled()) {
            return candidate;
        }
    }

    return nullptr;
}

void PagesUiTestFixture::click(QQuickItem *target)
{
    if (!target) {
        return;
    }

    QPointF stablePoint;
    bool haveStablePoint = false;
    QElapsedTimer stableFor;

    QTest::qWaitFor([&] {
        QQuickWindow *window = target->window();
        if (!window || !target->isVisible() || target->width() < 1 || target->height() < 1) {
            haveStablePoint = false;
            return false;
        }

        const QPointF point = target->mapToScene(QPointF(target->width() / 2.0, target->height() / 2.0));
        const bool inside = point.x() >= 0 && point.y() >= 0 && point.x() < window->width() && point.y() < window->height();
        const bool moved = !haveStablePoint || qAbs(point.x() - stablePoint.x()) > 1.0 || qAbs(point.y() - stablePoint.y()) > 1.0;

        if (!inside || moved) {
            stablePoint = point;
            haveStablePoint = inside;
            stableFor.restart();
            return false;
        }

        return stableFor.elapsed() >= 200;
    }, 5000);

    QQuickWindow *window = target->window();
    if (!window) {
        return;
    }

    window->requestActivate();
    QTest::qWaitForWindowExposed(window);

    const QPointF center = target->mapToScene(QPointF(target->width() / 2.0, target->height() / 2.0));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(50);
}

void PagesUiTestFixture::typeText(QQuickItem *target, const QString &text)
{
    if (!target || !m_window) {
        return;
    }

    click(target);

    for (const QChar character : text) {
        QTest::keyClick(m_window, character.toLatin1());
    }
}

void PagesUiTestFixture::clearAndType(QQuickItem *target, const QString &text)
{
    if (!target || !m_window) {
        return;
    }

    click(target);

    QTest::keySequence(m_window, QKeySequence::SelectAll);
    QTest::keyClick(m_window, Qt::Key_Backspace);

    typeText(target, text);
}

#include "pagesUiTestFixture.h"

#include <QCoreApplication>
#include <QEvent>
#include <QJSValue>
#include <QQuickStyle>
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

void PagesUiTestFixture::collectItems(QQuickItem *root, QList<QQuickItem *> &result)
{
    if (!root) {
        return;
    }

    result.append(root);

    for (QQuickItem *child : root->childItems()) {
        collectItems(child, result);
    }
}

QList<QQuickItem *> PagesUiTestFixture::items(QQuickItem *root) const
{
    QList<QQuickItem *> result;
    collectItems(root ? root : m_page, result);
    return result;
}

bool PagesUiTestFixture::isQmlType(const QObject *object, const QString &qmlType)
{
    return object && QString::fromLatin1(object->metaObject()->className()).startsWith(qmlType);
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

        if (candidate->isVisible() && candidateValue.isValid() && candidateValue == value) {
            return candidate;
        }
    }

    return nullptr;
}

QQuickItem *PagesUiTestFixture::clickable(const QString &text) const
{
    for (QQuickItem *candidate : items()) {
        const QVariant candidateText = candidate->property("text");

        if (candidateText.isValid() && candidateText.toString() == text && candidate->isVisible()
            && candidate->isEnabled()) {
            return candidate;
        }
    }

    return nullptr;
}

void PagesUiTestFixture::click(QQuickItem *target)
{
    if (!target || !m_window) {
        return;
    }

    const QPointF center = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));

    QTest::mouseClick(m_window, Qt::LeftButton, { }, center.toPoint());
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

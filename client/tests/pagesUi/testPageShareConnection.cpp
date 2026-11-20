#include <QQmlComponent>
#include <QQuickItem>
#include <QTest>
#include <QUrl>

#include "pagesUiTestFixture.h"

class TestPageShareConnection : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;
    QQuickItem *m_page = nullptr;

    bool loadPage()
    {
        if (!m_fixture.loadEmptyWindow()) {
            return false;
        }

        QQmlComponent component(m_fixture.engine(), QUrl(QStringLiteral("qrc:/ui/qml/Pages2/PageShareConnection.qml")));

        if (component.isError()) {
            qWarning() << component.errors();
            return false;
        }

        QVariantMap properties;
        properties.insert("headerText", QStringLiteral("Share"));
        properties.insert("configContentHeaderText", QStringLiteral("Connection settings"));
        properties.insert("isSelfHostedConfig", true);

        QObject *object = component.createWithInitialProperties(properties, m_fixture.engine()->rootContext());

        if (!object) {
            qWarning() << component.errors();
            return false;
        }

        m_page = qobject_cast<QQuickItem *>(object);

        if (!m_page) {
            delete object;
            return false;
        }

        m_page->setParentItem(m_fixture.window()->contentItem());
        m_page->setWidth(m_fixture.window()->width());
        m_page->setHeight(m_fixture.window()->height());

        return true;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanup()
    {
        delete m_page;
        m_page = nullptr;

        m_fixture.closePage();
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageLoads()
    {
        QVERIFY(loadPage());
        QVERIFY(m_page);
    }

    void shareControlsAreVisible()
    {
        QVERIFY(loadPage());
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Share"), m_page));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Copy"), m_page));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Show connection settings"), m_page));
    }

    void opensConnectionSettings()
    {
        QVERIFY(loadPage());

        QCOMPARE(m_page->width(), m_fixture.window()->width());
        QCOMPARE(m_page->height(), m_fixture.window()->height());

        QQuickItem *settingsButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Show connection settings"), m_page);

        QVERIFY(settingsButton);
        QVERIFY(settingsButton->isEnabled());
        QVERIFY(settingsButton->width() > 0);
        QVERIFY(settingsButton->height() > 0);

        m_fixture.click(settingsButton);

        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Connection settings")));
    }

    void nativeConfigStringButtonVisibility()
    {
        QVERIFY(loadPage());

        QQuickItem *copyStringButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Copy config string"), m_page);

        QVERIFY(!copyStringButton);
    }
};

int runPageShareConnectionTests(int argc, char *argv[])
{
    TestPageShareConnection test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageShareConnection.moc"

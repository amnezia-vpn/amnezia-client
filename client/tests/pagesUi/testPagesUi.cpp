#include <QApplication>
#include <QQuickStyle>
#include <QTest>

#include "pagesUiTestFixture.h"
#include "ui/utils/pageEnum.h"

class PagesUiTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanup()
    {
        m_fixture.closePage();
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageSetupWizardConfigSourceLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));
        QVERIFY(m_fixture.page());
    }

    void pageHomeLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));
        QVERIFY(m_fixture.page());
    }
};

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QApplication app(argc, argv);

    PageLoader::declareQmlPageEnum();

    qmlRegisterUncreatableMetaObject(
        ProtocolEnumNS::staticMetaObject,
        "ProtocolEnum",
        1,
        0,
        "ProtocolEnum",
        QStringLiteral("Error: only enums"));

    PagesUiTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPagesUi.moc"

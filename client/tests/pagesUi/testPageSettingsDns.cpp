#include <QQuickItem>
#include <QTest>
#include <QValidator>

#include "ui/controllers/settingsUiController.h"

#include "pagesUiTestFixture.h"

class PageSettingsDnsTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QQuickItem *fieldForHeader(const QString &header) const
    {
        const QList<QQuickItem *> fields = m_fixture.itemsOfType(QStringLiteral("TextFieldType"));

        for (QQuickItem *field : fields) {
            if (field->property("headerText").toString() == header) {
                return field;
            }
        }

        return nullptr;
    }

    QQuickItem *textInput(QQuickItem *field) const
    {
        if (!field) {
            return nullptr;
        }

        QObject *object = field->property("textField").value<QObject *>();
        return qobject_cast<QQuickItem *>(object);
    }

    QString fieldText(QQuickItem *field) const
    {
        QQuickItem *input = textInput(field);
        return input ? input->property("text").toString() : QString();
    }

    QValidator *fieldValidator(QQuickItem *field) const
    {
        QQuickItem *input = textInput(field);

        if (!input) {
            return nullptr;
        }

        QObject *validator = input->property("validator").value<QObject *>();
        return qobject_cast<QValidator *>(validator);
    }

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

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));
        QVERIFY(m_fixture.page());
    }

    void dnsFieldsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        QVERIFY(fieldForHeader(QStringLiteral("Primary DNS")));
        QVERIFY(fieldForHeader(QStringLiteral("Secondary DNS")));

        QVERIFY(m_fixture.clickable(QStringLiteral("Save")));
        QVERIFY(m_fixture.clickable(QStringLiteral("Restore default")));
    }

    void dnsFieldsDisplayCurrentSettings()
    {
        QVERIFY(m_fixture.core());
        QVERIFY(m_fixture.core()->m_settingsUiController);

        auto *settings = m_fixture.core()->m_settingsUiController;
        settings->setPrimaryDns(QStringLiteral("9.9.9.9"));
        settings->setSecondaryDns(QStringLiteral("149.112.112.112"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        QQuickItem *primaryDns = fieldForHeader(QStringLiteral("Primary DNS"));
        QQuickItem *secondaryDns = fieldForHeader(QStringLiteral("Secondary DNS"));

        QVERIFY(primaryDns);
        QVERIFY(secondaryDns);

        QCOMPARE(fieldText(primaryDns), QStringLiteral("9.9.9.9"));
        QCOMPARE(fieldText(secondaryDns), QStringLiteral("149.112.112.112"));
    }

    void dnsFieldsUseIpAddressValidator()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        const QStringList headers { QStringLiteral("Primary DNS"), QStringLiteral("Secondary DNS") };

        for (const QString &header : headers) {
            QQuickItem *field = fieldForHeader(header);

            QVERIFY2(field, qPrintable(header));

            QValidator *validator = fieldValidator(field);

            QVERIFY2(validator, qPrintable(header + QStringLiteral(" validator not found")));

            QString validAddress = QStringLiteral("1.1.1.1");
            int validPosition = 0;

            QCOMPARE(validator->validate(validAddress, validPosition), QValidator::Acceptable);

            QString invalidAddress = QStringLiteral("not-an-ip");
            int invalidPosition = 0;

            QCOMPARE(validator->validate(invalidAddress, invalidPosition), QValidator::Invalid);
        }
    }

    void primaryDnsCannotBeEmpty()
    {
        QVERIFY(m_fixture.core());
        QVERIFY(m_fixture.core()->m_settingsUiController);

        auto *settings = m_fixture.core()->m_settingsUiController;
        settings->setPrimaryDns(QStringLiteral("9.9.9.9"));
        settings->setSecondaryDns(QStringLiteral("149.112.112.112"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        QQuickItem *primaryDns = fieldForHeader(QStringLiteral("Primary DNS"));

        QVERIFY(primaryDns);

        QQuickItem *primaryInput = textInput(primaryDns);

        QVERIFY(primaryInput);

        m_fixture.clearAndType(primaryInput, QString());

        QQuickItem *saveButton = m_fixture.clickable(QStringLiteral("Save"));

        QVERIFY(saveButton);

        m_fixture.click(saveButton);

        QTRY_COMPARE(primaryDns->property("errorText").toString(), QStringLiteral("Primary DNS cannot be empty"));
        QCOMPARE(settings->getPrimaryDns(), QStringLiteral("9.9.9.9"));
    }

    void savesDnsSettings()
    {
        QVERIFY(m_fixture.core());
        QVERIFY(m_fixture.core()->m_settingsUiController);

        auto *settings = m_fixture.core()->m_settingsUiController;
        settings->setPrimaryDns(QStringLiteral("1.1.1.1"));
        settings->setSecondaryDns(QStringLiteral("1.0.0.1"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        QQuickItem *primaryDns = fieldForHeader(QStringLiteral("Primary DNS"));

        QQuickItem *secondaryDns = fieldForHeader(QStringLiteral("Secondary DNS"));

        QVERIFY(primaryDns);
        QVERIFY(secondaryDns);

        QQuickItem *primaryInput = textInput(primaryDns);
        QQuickItem *secondaryInput = textInput(secondaryDns);

        QVERIFY(primaryInput);
        QVERIFY(secondaryInput);

        m_fixture.clearAndType(primaryInput, QStringLiteral("9.9.9.9"));
        m_fixture.clearAndType(secondaryInput, QStringLiteral("149.112.112.112"));

        QQuickItem *saveButton = m_fixture.clickable(QStringLiteral("Save"));

        QVERIFY(saveButton);

        m_fixture.click(saveButton);

        QTRY_COMPARE(settings->getPrimaryDns(), QStringLiteral("9.9.9.9"));
        QTRY_COMPARE(settings->getSecondaryDns(), QStringLiteral("149.112.112.112"));

        QCOMPARE(primaryDns->property("errorText").toString(), QString());
        QCOMPARE(secondaryDns->property("errorText").toString(), QString());
    }

    void secondaryDnsCanBeEmpty()
    {
        QVERIFY(m_fixture.core());
        QVERIFY(m_fixture.core()->m_settingsUiController);

        auto *settings = m_fixture.core()->m_settingsUiController;
        settings->setPrimaryDns(QStringLiteral("1.1.1.1"));
        settings->setSecondaryDns(QStringLiteral("1.0.0.1"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));

        QQuickItem *secondaryDns = fieldForHeader(QStringLiteral("Secondary DNS"));

        QVERIFY(secondaryDns);

        QQuickItem *secondaryInput = textInput(secondaryDns);

        QVERIFY(secondaryInput);

        m_fixture.clearAndType(secondaryInput, QString());

        QQuickItem *saveButton = m_fixture.clickable(QStringLiteral("Save"));

        QVERIFY(saveButton);

        m_fixture.click(saveButton);

        QTRY_COMPARE(settings->getSecondaryDns(), QString());
        QCOMPARE(settings->getPrimaryDns(), QStringLiteral("1.1.1.1"));
    }
};

int runPageSettingsDnsTests(int argc, char *argv[])
{
    PageSettingsDnsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsDns.moc"

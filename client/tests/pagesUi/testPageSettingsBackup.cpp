#include <QFile>
#include <QJSValue>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

#include "ui/controllers/settingsUiController.h"

#include "pagesUiTestFixture.h"

class TestPageSettingsBackup : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QObject *dialogs() const
    {
        if (!m_fixture.engine() || m_fixture.engine()->rootObjects().isEmpty()) {
            return nullptr;
        }

        return m_fixture.engine()->rootObjects().last()->property("dialogs").value<QObject *>();
    }

    bool invokeRestoreBackup(const QString &filePath)
    {
        return QMetaObject::invokeMethod(m_fixture.page(), "restoreBackup", Q_ARG(QVariant, QVariant(filePath)));
    }

    QJSValue callbackFromSignal(const QList<QVariant> &arguments, int index)
    {
        if (index >= arguments.size()) {
            return { };
        }

        return arguments.at(index).value<QJSValue>();
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));
        QVERIFY(m_fixture.page());
    }

    void backupInformationIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        QQuickItem *title = m_fixture.visibleItem(QStringLiteral("title"), QStringLiteral("Back up your configuration"));

        QVERIFY2(title, "Backup page title was not found");

        QQuickItem *warning =
                m_fixture.visibleItem(QStringLiteral("text"),
                                      QStringLiteral("The backup will contain your passwords and private keys "
                                                     "for all servers added to AmneziaVPN. Keep this information "
                                                     "in a secure place."));

        QVERIFY2(warning, "Backup security warning was not found");
    }

    void backupButtonsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        QQuickItem *makeBackup = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Make a backup"));
        QQuickItem *restoreBackup = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Restore from backup"));

        QVERIFY2(makeBackup, "Make a backup button was not found");
        QVERIFY2(restoreBackup, "Restore from backup button was not found");

        QVERIFY(makeBackup->isEnabled());
        QVERIFY(restoreBackup->isEnabled());
    }

    void restoreBackupRequestsConfirmation()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        QObject *dialogObject = dialogs();

        QVERIFY(dialogObject);

        QSignalSpy questionSpy(dialogObject, SIGNAL(questionRequested(QString, QString, QString, QString, QVariant, QVariant)));

        QVERIFY(questionSpy.isValid());
        QVERIFY(invokeRestoreBackup(QStringLiteral("test.backup")));
        QTRY_COMPARE(questionSpy.count(), 1);

        const QList<QVariant> arguments = questionSpy.takeFirst();

        QCOMPARE(arguments.at(0).toString(), QStringLiteral("Import settings from a backup file?"));
        QCOMPARE(arguments.at(1).toString(), QStringLiteral("All current settings will be reset"));
        QCOMPARE(arguments.at(2).toString(), QStringLiteral("Continue"));
        QCOMPARE(arguments.at(3).toString(), QStringLiteral("Cancel"));
    }

    void cancellingRestoreDoesNotRestoreBackup()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        QObject *dialogObject = dialogs();

        QVERIFY(dialogObject);

        QSignalSpy questionSpy(dialogObject, SIGNAL(questionRequested(QString, QString, QString, QString, QVariant, QVariant)));
        QSignalSpy restoreSpy(m_fixture.core()->m_settingsUiController, &SettingsUiController::restoreBackupFinished);

        QVERIFY(invokeRestoreBackup(QStringLiteral("test.backup")));
        QTRY_COMPARE(questionSpy.count(), 1);

        const QList<QVariant> arguments = questionSpy.takeFirst();

        QJSValue cancelCallback = callbackFromSignal(arguments, 5);

        QVERIFY(cancelCallback.isCallable());

        cancelCallback.call();

        QCoreApplication::processEvents();

        QCOMPARE(restoreSpy.count(), 0);
        QCOMPARE(m_fixture.page()->property("isRestoringBackup").toBool(), false);
    }

    void restoreButtonIsDisabledWhileRestoring()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        QQuickItem *restoreButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Restore from backup"));

        QVERIFY(restoreButton);
        QCOMPARE(m_fixture.page()->property("isRestoringBackup").toBool(), false);
        QVERIFY(restoreButton->isEnabled());
    }

    void validBackupCanBeRestored()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsBackup")));

        const QByteArray backupData = m_fixture.core()->m_settingsController->backupAppConfig();

        QVERIFY2(!backupData.isEmpty(), "Could not create test backup data");

        QTemporaryFile backupFile;

        QVERIFY(backupFile.open());
        QCOMPARE(backupFile.write(backupData), qint64(backupData.size()));

        backupFile.flush();

        const QString backupPath = backupFile.fileName();

        QObject *dialogObject = dialogs();

        QVERIFY(dialogObject);

        QSignalSpy questionSpy(dialogObject, SIGNAL(questionRequested(QString, QString, QString, QString, QVariant, QVariant)));
        QSignalSpy restoreSpy(m_fixture.core()->m_settingsUiController, &SettingsUiController::restoreBackupFinished);

        QVERIFY(questionSpy.isValid());
        QVERIFY(restoreSpy.isValid());
        QVERIFY(invokeRestoreBackup(backupPath));
        QTRY_COMPARE(questionSpy.count(), 1);

        const QList<QVariant> arguments = questionSpy.takeFirst();

        QJSValue continueCallback = callbackFromSignal(arguments, 4);

        QVERIFY(continueCallback.isCallable());

        continueCallback.call();

        QVERIFY(QTest::qWaitFor([&restoreSpy] { return restoreSpy.count() == 1; }, 10000));
        QCOMPARE(restoreSpy.count(), 1);
        QCOMPARE(m_fixture.page()->property("isRestoringBackup").toBool(), false);
    }
};

int runPageSettingsBackupTests(int argc, char *argv[])
{
    TestPageSettingsBackup test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsBackup.moc"

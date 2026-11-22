#include <QProcess>
#include <QStandardPaths>
#include <QTest>

#include "core/utils/selfhosted/sshClient.h"

class TestSshShellCommand : public QObject
{
    Q_OBJECT

private:
    struct ShellResult
    {
        int exitCode = -1;
        QByteArray stdOut;
        QByteArray stdErr;
    };

    static ShellResult runShell(const QString &shell, const QString &command)
    {
        QProcess process;
        process.start(shell, { QStringLiteral("-c"), command });
        process.waitForFinished();
        return { process.exitCode(), process.readAllStandardOutput(), process.readAllStandardError() };
    }

private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#endif
    }

    void runsUnderAnyLoginShell_data()
    {
        QTest::addColumn<QString>("loginShell");
        QTest::addColumn<QString>("command");

        const QMap<QString, QString> commands = {
            { "assignment", "LOCK_CMD=\"fuser\"; LOCK_FILE='/var/lib/dpkg/lock-frontend'; echo \"$LOCK_CMD $LOCK_FILE\"" },
            { "command substitution", "CUR_USER=$(echo $HOME | sed 's/.*\\///'); echo \"$CUR_USER\"" },
            { "continued lines", "if [ \"a\" = \"a\" ]; then \\\n  echo yes;\\\nfi" },
            { "quotes and backslashes", "printf '%s|%s\\n' 'it'\\''s' 'back\\\\slash'" },
            { "parameter expansion", "v=6.8.0-147; v=${v%%-*}; echo \"$v\"" },
            { "exit status", "false || echo recovered; exit 3" },
        };

        for (const QString &shellName : { "sh", "bash", "dash", "zsh", "fish" }) {
            const QString shell = QStandardPaths::findExecutable(shellName);
            if (shell.isEmpty()) {
                continue;
            }
            for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
                QTest::addRow("%s: %s", qPrintable(shellName), qPrintable(it.key())) << shell << it.value();
            }
        }
    }

    void runsUnderAnyLoginShell()
    {
        QFETCH(QString, loginShell);
        QFETCH(QString, command);

        const ShellResult expected = runShell(QStandardPaths::findExecutable("sh"), command);
        const ShellResult actual = runShell(loginShell, libssh::Client::posixShellCommand(command));

        QCOMPARE(actual.stdErr, expected.stdErr);
        QCOMPARE(actual.stdOut, expected.stdOut);
        QCOMPARE(actual.exitCode, expected.exitCode);
    }
};

QTEST_MAIN(TestSshShellCommand)
#include "testSshShellCommand.moc"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>

#include "core/controllers/selfhosted/installController.h"
#include "logger.h"

using namespace amnezia;

namespace
{
    const QString oldSecret(32, QLatin1Char('1'));
    const QString firstSecret = QStringLiteral("0123456789abcdef0123456789abcdef");
    const QString secondSecret = QStringLiteral("fedcba9876543210fedcba9876543210");

    QString tgLink(const QString &secret)
    {
        return QStringLiteral("tg://proxy?server=proxy.example&port=443&secret=%1").arg(secret);
    }

    QString tmeLink(const QString &secret)
    {
        return QStringLiteral("https://t.me/proxy?server=proxy.example&port=443&secret=%1").arg(secret);
    }

    QString configureOutput(const QString &secret, const QString &linkSecret)
    {
        return QStringLiteral("[*] Secret:    %1\n[*] tg:// link:   %2\n[*] t.me link:    %3\n")
                .arg(secret, tgLink(linkSecret), tmeLink(linkSecret));
    }

    class MessageCapture
    {
    public:
        MessageCapture()
        {
            current = this;
            previous = qInstallMessageHandler(handleMessage);
        }

        ~MessageCapture()
        {
            qInstallMessageHandler(previous);
            current = nullptr;
        }

        QString messages;

    private:
        static void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
        {
            current->messages += message + QLatin1Char('\n');
            if (current->previous) {
                current->previous(type, context, message);
            }
        }

        inline static MessageCapture *current = nullptr;
        QtMessageHandler previous = nullptr;
    };
}

class TestProxyConfigurationLogging : public QObject
{
    Q_OBJECT

private:
    QtMessageHandler m_previousHandler = nullptr;

    static ContainerConfig initialConfig(DockerContainer container)
    {
        const auto protocol = [](auto config) {
            config.port = QStringLiteral("443");
            config.secret = oldSecret;
            config.tgLink = tgLink(oldSecret);
            config.tmeLink = tmeLink(oldSecret);
            config.publicHost = QStringLiteral("proxy.example");
            config.transportMode = QStringLiteral("standard");
            config.additionalSecrets = { secondSecret };
            return ProtocolConfig(config);
        };

        ContainerConfig config;
        config.container = container;
        config.protocolConfig = container == DockerContainer::MtProxy ? protocol(MtProxyProtocolConfig { })
                                                                      : protocol(TelemtProtocolConfig { });
        return config;
    }

    static QJsonObject expectedConfig(ContainerConfig config, const QString &secret, const QString &tg, const QString &tme)
    {
        const auto update = [&](auto *protocol) {
            protocol->secret = secret;
            protocol->tgLink = tg;
            protocol->tmeLink = tme;
        };
        if (config.container == DockerContainer::MtProxy) {
            update(config.getMtProxyProtocolConfig());
        } else {
            update(config.getTelemtProtocolConfig());
        }
        return config.toJson();
    }

    static void verifyLogs(const QString &messages)
    {
        QFile file(Logger::userLogsFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString fileLog = QString::fromUtf8(file.readAll());
        QVERIFY(messages.contains(QStringLiteral("proxy logging control")));
        QVERIFY(fileLog.contains(QStringLiteral("proxy logging control")));

        const QStringList forbidden = { oldSecret,
                                        firstSecret,
                                        secondSecret,
                                        QStringLiteral("tg://proxy?"),
                                        QStringLiteral("https://t.me/proxy?"),
                                        QStringLiteral("raw-output-canary") };
        for (const QString &value : forbidden) {
            QVERIFY2(!fileLog.contains(value), "Credentials or raw command output reached the log file");
            QVERIFY2(!messages.contains(value), "Credentials or raw command output reached the Qt message handler");
        }
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(
                QStringLiteral("AmneziaProxyLoggingTest-%1").arg(QUuid::createUuid().toString()));
        m_previousHandler = qInstallMessageHandler(nullptr);
        QVERIFY(Logger::init(false));
    }

    void init()
    {
        Logger::clearLogs(false);
    }

    void cleanupTestCase()
    {
        const QString path = Logger::userLogsFilePath();
        Logger::deInit();
        qInstallMessageHandler(m_previousHandler);
        QVERIFY(QFile::remove(path));
        QDir directory = QFileInfo(path).absoluteDir();
        const QString logDirectory = directory.absolutePath();
        QVERIFY(directory.cdUp());
        QDir().rmdir(logDirectory);
        QDir().rmdir(directory.absolutePath());
    }

    void parseWithoutLoggingCredentials_data()
    {
        QTest::addColumn<int>("containerValue");
        QTest::addColumn<QString>("output");
        QTest::addColumn<QString>("secret");
        QTest::addColumn<QString>("tg");
        QTest::addColumn<QString>("tme");

        for (DockerContainer container : { DockerContainer::MtProxy, DockerContainer::Telemt }) {
            const QByteArray prefix = container == DockerContainer::MtProxy ? "mtproxy-" : "telemt-";
            const auto row = [&](const char *name, const QString &output, const QString &secret, const QString &tg,
                                 const QString &tme) {
                QTest::newRow((prefix + name).constData())
                        << static_cast<int>(container) << output << secret << tg << tme;
            };
            const QString padded = QStringLiteral("dd") + firstSecret;
            const QString fakeTls = QStringLiteral("ee") + firstSecret + QStringLiteral("6578616d706c652e6f7267");
            row("padded", configureOutput(firstSecret, padded), firstSecret, tgLink(padded), tmeLink(padded));
            row("faketls", configureOutput(firstSecret, fakeTls), firstSecret, tgLink(fakeTls), tmeLink(fakeTls));
            row("secret-only",
                QStringLiteral("[*] Secret:    %1\n[*] tg:// link:   \n[*] t.me link:    \n").arg(firstSecret),
                firstSecret, tgLink(oldSecret), tmeLink(oldSecret));
            row("links-only",
                QStringLiteral("[*] tg:// link: %1\n[*] t.me link: %2\n").arg(tgLink(padded), tmeLink(padded)),
                oldSecret, tgLink(padded), tmeLink(padded));
            row("empty", { }, oldSecret, tgLink(oldSecret), tmeLink(oldSecret));
            row("incomplete", QStringLiteral("[*] Secret: 01234567\n[*] tg:// link:\n"), oldSecret, tgLink(oldSecret),
                tmeLink(oldSecret));
            row("invalid", QStringLiteral("[*] Secret: not-hex\n[*] tg:// link: invalid\n[*] t.me link: invalid\n"),
                oldSecret, tgLink(oldSecret), tmeLink(oldSecret));
            row("unmarked", firstSecret + QLatin1Char('\n') + tgLink(padded) + QLatin1Char('\n') + tmeLink(padded),
                oldSecret, tgLink(oldSecret), tmeLink(oldSecret));
            row("noisy", QStringLiteral("raw-output-canary\n") + configureOutput(firstSecret, padded), firstSecret,
                tgLink(padded), tmeLink(padded));
        }
    }

    void parseWithoutLoggingCredentials()
    {
        QFETCH(int, containerValue);
        QFETCH(QString, output);
        QFETCH(QString, secret);
        QFETCH(QString, tg);
        QFETCH(QString, tme);

        const auto container = static_cast<DockerContainer>(containerValue);
        ContainerConfig config = initialConfig(container);
        const QJsonObject expected = expectedConfig(config, secret, tg, tme);
        MessageCapture capture;
        qDebug() << "proxy logging control";
        InstallController::updateContainerConfigAfterInstallation(container, config, output);
        QCOMPARE(config.toJson(), expected);
        verifyLogs(capture.messages);
    }

    void repeatedConfiguration_data()
    {
        QTest::addColumn<int>("containerValue");
        QTest::newRow("mtproxy") << static_cast<int>(DockerContainer::MtProxy);
        QTest::newRow("telemt") << static_cast<int>(DockerContainer::Telemt);
    }

    void repeatedConfiguration()
    {
        QFETCH(int, containerValue);
        const auto container = static_cast<DockerContainer>(containerValue);
        ContainerConfig config = initialConfig(container);
        MessageCapture capture;
        qDebug() << "proxy logging control";
        for (const QString &secret : { firstSecret, secondSecret }) {
            const QString padded = QStringLiteral("dd") + secret;
            const QJsonObject expected = expectedConfig(config, secret, tgLink(padded), tmeLink(padded));
            InstallController::updateContainerConfigAfterInstallation(container, config, configureOutput(secret, padded));
            QCOMPARE(config.toJson(), expected);
        }
        verifyLogs(capture.messages);
    }
};

QTEST_GUILESS_MAIN(TestProxyConfigurationLogging)
#include "testProxyConfigurationLogging.moc"

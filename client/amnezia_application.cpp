#include "amnezia_application.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QMimeData>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QTranslator>

#include "core/servercontroller.h"
#include "logger.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"

#include "ui/pages.h"

#if defined(Q_OS_IOS)
    #include "platforms/ios/QtAppDelegate-C-Interface.h"
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv)
#else
AmneziaApplication::AmneziaApplication(int &argc, char *argv[], bool allowSecondary, SingleApplication::Options options,
                                       int timeout, const QString &userData)
    : SingleApplication(argc, argv, allowSecondary, options, timeout, userData)
#endif
{
    setQuitOnLastWindowClosed(false);

    // Fix config file permissions
#ifdef Q_OS_LINUX && !defined(Q_OS_ANDROID)
    {
        QSettings s(ORGANIZATION_NAME, APPLICATION_NAME);
        s.setValue("permFixed", true);
    }

    QString configLoc1 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/"
            + ORGANIZATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc1, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QString configLoc2 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/"
            + ORGANIZATION_NAME + "/" + APPLICATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc2, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    m_settings = std::shared_ptr<Settings>(new Settings);
}

AmneziaApplication::~AmneziaApplication()
{
    if (m_engine) {
        QObject::disconnect(m_engine, 0, 0, 0);
        delete m_engine;
    }

    if (m_protocolProps)
        delete m_protocolProps;
    if (m_containerProps)
        delete m_containerProps;
}

void AmneziaApplication::init()
{
    m_engine = new QQmlApplicationEngine;

    const QUrl url(QStringLiteral("qrc:/ui/qml/main2.qml"));
    QObject::connect(
            m_engine, &QQmlApplicationEngine::objectCreated, this,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

    //
    m_containersModel.reset(new ContainersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());
    connect(m_serversModel.get(), &ServersModel::currentlyProcessedServerIndexChanged, m_containersModel.get(),
            &ContainersModel::setCurrentlyProcessedServerIndex);

    m_languageModel.reset(new LanguageModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());
    connect(m_languageModel.get(), &LanguageModel::updateTranslations, this, &AmneziaApplication::updateTranslator);

    m_configurator = std::shared_ptr<VpnConfigurator>(new VpnConfigurator(m_settings, this));
    m_vpnConnection.reset(new VpnConnection(m_settings, m_configurator));

    m_connectionController.reset(new ConnectionController(m_serversModel, m_containersModel, m_vpnConnection));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversModel, m_containersModel, m_settings, m_configurator));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(new SettingsController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());

    //

    m_engine->load(url);

    //    if (m_engine->rootObjects().size() > 0) {
    //        m_uiLogic->setQmlRoot(m_engine->rootObjects().at(0));
    //    }

    if (m_settings->isSaveLogs()) {
        if (!Logger::init()) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }

    // #ifdef Q_OS_WIN
    //     if (m_parser.isSet("a")) m_uiLogic->showOnStartup();
    //     else emit m_uiLogic->show();
    // #else
    //     m_uiLogic->showOnStartup();
    // #endif

//    // TODO - fix
// #if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
//    if (isPrimary()) {
//        QObject::connect(this, &SingleApplication::instanceStarted, m_uiLogic, [this](){
//            qDebug() << "Secondary instance started, showing this window instead";
//            emit m_uiLogic->show();
//            emit m_uiLogic->raise();
//        });
//    }
// #endif

// Android TextField clipboard workaround
// https://bugreports.qt.io/browse/QTBUG-113461
#ifdef Q_OS_ANDROID
    QObject::connect(qApp, &QApplication::applicationStateChanged, [](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            if (qApp->clipboard()->mimeData()->formats().contains("text/html")) {
                QTextDocument doc;
                doc.setHtml(qApp->clipboard()->mimeData()->html());
                qApp->clipboard()->setText(doc.toPlainText());
            }
        }
    });
#endif
}

void AmneziaApplication::registerTypes()
{
    qRegisterMetaType<ServerCredentials>("ServerCredentials");

    qRegisterMetaType<DockerContainer>("DockerContainer");
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Proto>("Proto");
    qRegisterMetaType<ServiceType>("ServiceType");

    declareQmlProtocolEnum();
    declareQmlContainerEnum();

    qmlRegisterType<QRCodeReader>("QRCodeReader", 1, 0, "QRCodeReader");

    m_containerProps = new ContainerProps;
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", m_containerProps);

    m_protocolProps = new ProtocolProps;
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", m_protocolProps);

    qmlRegisterSingletonType(QUrl("qrc:/ui/qml/Filters/ContainersModelFilters.qml"), "ContainersModelFilters", 1, 0,
                             "ContainersModelFilters");

    //
    Vpn::declareQmlVpnConnectionStateEnum();
    PageLoader::declareQmlPageEnum();
}

void AmneziaApplication::loadFonts()
{
    QQuickStyle::setStyle("Basic");

    QFontDatabase::addApplicationFont(":/fonts/pt-root-ui_vf.ttf");
}

void AmneziaApplication::loadTranslator()
{
    auto locale = m_settings->getAppLanguage();
    m_translator = new QTranslator;
    if (m_translator->load(locale, QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        installTranslator(m_translator);
    }
}

void AmneziaApplication::updateTranslator(const QLocale &locale)
{
    QResource::registerResource(":/translations.qrc");
    if (!m_translator->isEmpty())
        QCoreApplication::removeTranslator(m_translator);
    if (m_translator->load(locale, QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        if (QCoreApplication::installTranslator(m_translator)) {
            m_settings->setAppLanguage(locale);
        }

        m_engine->retranslate();
    }
}

bool AmneziaApplication::parseCommands()
{
    m_parser.setApplicationDescription(APPLICATION_NAME);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption c_autostart { { "a", "autostart" }, "System autostart" };
    m_parser.addOption(c_autostart);

    QCommandLineOption c_cleanup { { "c", "cleanup" }, "Cleanup logs" };
    m_parser.addOption(c_cleanup);

    m_parser.process(*this);

    if (m_parser.isSet(c_cleanup)) {
        Logger::cleanUp();
        QTimer::singleShot(100, this, [this] { quit(); });
        exec();
        return false;
    }
    return true;
}

QQmlApplicationEngine *AmneziaApplication::qmlEngine() const
{
    return m_engine;
}

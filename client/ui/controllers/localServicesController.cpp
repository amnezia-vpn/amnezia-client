#include "localServicesController.h"

namespace
{
    // Logger logger("ServerController");
}

LocalServicesController::LocalServicesController(const QSharedPointer<ServersModel> &serversModel,
                                                 const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_settings(settings)
{
    connect(&m_goodbyeDpiService, &GoodByeDpi::serviceStateChanged, this, &LocalServicesController::serviceStateChanged);
}

LocalServicesController::~LocalServicesController()
{
    m_goodbyeDpiService.stop();
}

void LocalServicesController::toggleGoodbyeDpi(bool enable)
{
    if (enable) {
        // auto file = getGoodbyeDpiBlackListFile();
        // auto modset = getGoodbyeDpiModset();
        // auto errorCode = ErrorCode::NoError;//m_goodbyeDpiService.start(file, modset);
        // if (errorCode != ErrorCode::NoError) {
        //     emit errorOccurred(errorCode);
        // } else {

        QJsonObject server;
        server.insert(config_key::isGoodbyeDpi, true);
        server.insert(config_key::description, "GoodbyeDPI service");
        server.insert(config_key::name, "GoodbyeDPI");
        m_serversModel->addServer(server);
        m_serversModel->setDefaultServerIndex(m_serversModel->getServersCount() - 1);

        m_settings->toggleGoodbyeDpi(true);
        emit toggleGoodbyeDpiFinished(tr("GoodbyeDPI added to home page"));
        // }
    } else {
        // m_goodbyeDpiService.stop();

        for (int i = 0; i < m_serversModel->getServersCount(); i++) {
            if (m_serversModel->getServerConfig(i).value(config_key::isGoodbyeDpi).toBool(false)) {
                m_serversModel->setProcessedServerIndex(i);
                m_serversModel->removeServer();
                break;
            }
        }

        m_settings->toggleGoodbyeDpi(false);
        emit toggleGoodbyeDpiFinished("GoodbyeDPI removed from home page");
    }
}

bool LocalServicesController::isGoodbyeDpiEnabled()
{
    return m_settings->isGoodbyeDpiEnabled();
}

void LocalServicesController::setGoodbyeDpiBlackListFile(const QString &file)
{
    m_settings->setGoodbyeDpiBlackListFile(file);
}

QString LocalServicesController::getGoodbyeDpiBlackListFile()
{
    auto file = m_settings->getGoodbyeDpiBlackListFile();
    if (file.isEmpty()) {
        return m_defaultBlackListFile;
    }
    return file;
}

void LocalServicesController::resetGoodbyeDpiBlackListFile()
{
    m_settings->setGoodbyeDpiBlackListFile(m_defaultBlackListFile);
}

void LocalServicesController::setGoodbyeDpiModset(const int modset)
{
    m_settings->setGoodbyeDpiModset(modset);
}

int LocalServicesController::getGoodbyeDpiModset()
{
    return m_settings->getGoodbyeDpiModset();
}

void LocalServicesController::start()
{
    auto errorCode = m_goodbyeDpiService.start(getGoodbyeDpiBlackListFile(), getGoodbyeDpiModset());
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
    }
}

void LocalServicesController::stop()
{
    m_goodbyeDpiService.stop();
}

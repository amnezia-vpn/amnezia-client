#include "pageController.h"

PageController::PageController(const QSharedPointer<ServersModel> &serversModel,
                               QObject *parent) : QObject(parent), m_serversModel(serversModel)
{
}

QString PageController::getInitialPage()
{
    if (m_serversModel->getServersCount()) {
        if (m_serversModel->getDefaultServerIndex() < 0) {
            auto defaultServerIndex = m_serversModel->index(0);
            m_serversModel->setData(defaultServerIndex, true, ServersModel::Roles::IsDefaultRole);
        }
        return getPagePath(PageLoader::PageEnum::PageStart);
    } else {
        return getPagePath(PageLoader::PageEnum::PageSetupWizardStart);
    }
}

QString PageController::getPagePath(PageLoader::PageEnum page)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PageLoader::PageEnum>();
    QString pageName = metaEnum.valueToKey(static_cast<int>(page));
    return "qrc:/ui/qml/Pages2/" + pageName + ".qml";
}

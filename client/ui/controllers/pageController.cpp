#include "pageController.h"

PageController::PageController(const QSharedPointer<ServersModel> &serversModel,
                               QObject *parent) : QObject(parent), m_serversModel(serversModel)
{
}

void PageController::setStartPage()
{
    if (m_serversModel->getServersCount()) {
        if (m_serversModel->getDefaultServerIndex() < 0) {
            m_serversModel->setDefaultServerIndex(0);
        }
        emit goToPage(PageLoader::PageEnum::PageStart, false);
    } else {
        emit goToPage(PageLoader::PageEnum::PageSetupWizardStart, false);
    }
}

QString PageController::getPagePath(PageLoader::PageEnum page)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PageLoader::PageEnum>();
    QString pageName = metaEnum.valueToKey(static_cast<int>(page));
    return "Pages2/" + pageName + ".qml";
}

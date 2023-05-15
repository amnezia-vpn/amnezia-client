#ifndef PAGECONTROLLER_H
#define PAGECONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "ui/models/servers_model.h"

namespace PageLoader
{
    Q_NAMESPACE
    enum class PageEnum { PageStart = 0, PageHome, PageSettings, PageShare,

                          PageSetupWizardStart, PageTest, PageSetupWizardCredentials, PageSetupWizardProtocols, PageSetupWizardEasy,
                          PageSetupWizardProtocolSettings, PageSetupWizardInstalling, PageSetupWizardConfigSource,
                          PageSetupWizardTextKey
    };
    Q_ENUM_NS(PageEnum)

    static void declareQmlPageEnum() {
        qmlRegisterUncreatableMetaObject(
            PageLoader::staticMetaObject,
            "PageEnum",
            1, 0,
            "PageEnum",
            "Error: only enums"
            );
    }
}

class PageController : public QObject
{
    Q_OBJECT
public:
    explicit PageController(const QSharedPointer<ServersModel> &serversModel,
                            QObject *parent = nullptr);

public slots:
    void setStartPage();
    QString getPagePath(PageLoader::PageEnum page);

signals:
    void goToPage(PageLoader::PageEnum page, bool slide = true);
    void closePage();

private:
    QSharedPointer<ServersModel> m_serversModel;
};

#endif // PAGECONTROLLER_H

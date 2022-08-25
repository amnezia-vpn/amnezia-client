#ifndef PAGE_LOGIC_BASE_H
#define PAGE_LOGIC_BASE_H

#include "../pages.h"
#include "../property_helper.h"

using namespace PageEnumNS;

class UiLogic;
class Settings;
class VpnConfigurator;
class ServerController;

class PageLogicBase : public QObject
{
    Q_OBJECT
    AUTO_PROPERTY(bool, pageEnabled)

public:
    explicit PageLogicBase(UiLogic *uiLogic, QObject *parent = nullptr);
    ~PageLogicBase() = default;

    Q_INVOKABLE virtual void onUpdatePage() {}

protected:
    UiLogic *m_uiLogic;
    UiLogic *uiLogic() const { return m_uiLogic; }

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;
    std::shared_ptr<ServerController> m_serverController;

signals:
    void updatePage();
};
#endif // PAGE_LOGIC_BASE_H

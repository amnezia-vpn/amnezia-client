#ifndef PAGE_LOGIC_BASE_H
#define PAGE_LOGIC_BASE_H

#include "settings.h"
#include "../pages.h"
#include "../property_helper.h"

using namespace amnezia;
using namespace PageEnumNS;

class UiLogic;

class PageLogicBase : public QObject
{
    Q_OBJECT
    AUTO_PROPERTY(bool, pageEnabled)

public:
    explicit PageLogicBase(UiLogic *uiLogic, QObject *parent = nullptr);
    ~PageLogicBase() = default;

    Q_INVOKABLE virtual void updatePage() {}
    Q_INVOKABLE Page pageForProto(amnezia::Protocol p);

protected:
    UiLogic *uiLogic() const { return m_uiLogic; }

    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // PAGE_LOGIC_BASE_H

#ifndef PAGE_LOGIC_BASE_H
#define PAGE_LOGIC_BASE_H

#include "../pages.h"
#include "settings.h"

using namespace amnezia;
using namespace PageEnumNS;

class UiLogic;

class PageLogicBase : public QObject
{
    Q_OBJECT

public:
    explicit PageLogicBase(UiLogic *uiLogic, QObject *parent = nullptr);
    ~PageLogicBase() = default;

    Q_INVOKABLE void updatePage() {}

protected:
    UiLogic *uiLogic() const { return m_uiLogic; }

    Settings m_settings;
    UiLogic *m_uiLogic;

signals:

private slots:


private:


};
#endif // PAGE_LOGIC_BASE_H

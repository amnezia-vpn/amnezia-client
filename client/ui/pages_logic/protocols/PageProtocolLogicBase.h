#ifndef PAGE_PROTOCOL_LOGIC_BASE_H
#define PAGE_PROTOCOL_LOGIC_BASE_H

#include "settings.h"
#include "../PageLogicBase.h"

using namespace amnezia;
using namespace PageEnumNS;

class UiLogic;

class PageProtocolLogicBase : public PageLogicBase
{
    Q_OBJECT

public:
    explicit PageProtocolLogicBase(UiLogic *uiLogic, QObject *parent = nullptr);
    ~PageProtocolLogicBase() = default;

    virtual void updateProtocolPage(const QJsonObject &config, DockerContainer container, bool haveAuthData) {}
    virtual QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) { return QJsonObject(); }

};
#endif // PAGE_PROTOCOL_LOGIC_BASE_H

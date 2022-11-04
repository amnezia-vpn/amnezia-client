#ifndef WIREGUARDLOGIC_H
#define WIREGUARDLOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class WireGuardLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, wireGuardLastConfigText)
    AUTO_PROPERTY(bool, isThirdPartyConfig)

public:
    explicit WireGuardLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~WireGuardLogic() = default;

    void updateProtocolPage(const QJsonObject &wireGuardConfig, DockerContainer container, bool haveAuthData, bool isThirdPartyConfig) override;

private:
    UiLogic *m_uiLogic;

};

#endif // WIREGUARDLOGIC_H

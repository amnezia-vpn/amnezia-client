#ifndef NEW_SERVER_PROTOCOLS_LOGIC_H
#define NEW_SERVER_PROTOCOLS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class NewServerProtocolsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(double, progressBarConnectionMinimum)
    AUTO_PROPERTY(double, progressBarConnectionMaximum)

public:
    Q_INVOKABLE void onUpdatePage() override;
    Q_INVOKABLE void onPushButtonConfigureClicked(DockerContainer c, int port, TransportProto tp);

public:
    explicit NewServerProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerProtocolsLogic() = default;

};
#endif // NEW_SERVER_PROTOCOLS_LOGIC_H

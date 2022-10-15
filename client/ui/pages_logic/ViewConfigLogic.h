#ifndef VIEW_CONFIG_LOGIC_H
#define VIEW_CONFIG_LOGIC_H

#include "PageLogicBase.h"

#include <QJsonObject>

class UiLogic;

class ViewConfigLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, configText)
    AUTO_PROPERTY(QString, openVpnLastConfigs)
    AUTO_PROPERTY(QString, openVpnMalStrings)
    AUTO_PROPERTY(QJsonObject, configJson)
    AUTO_PROPERTY(int, warningStringNumber)
    AUTO_PROPERTY(bool, warningActive)

public:
    Q_INVOKABLE void onUpdatePage() override;
    Q_INVOKABLE void importConfig();


public:
    explicit ViewConfigLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ViewConfigLogic() = default;

private:
    QRegularExpression m_re {"(\\w+-\\w+|\\w+)"};

    // https://github.com/OpenVPN/openvpn/blob/master/doc/man-sections/script-options.rst
    QStringList dangerousTags {
        "up",
        "tls-verify",
        "ipchange",
        "client-connect",
        "route-up",
        "route-pre-down",
        "client-disconnect",
        "down",
        "learn-address",
        "auth-user-pass-verify"
    };
};
#endif // VIEW_CONFIG_LOGIC_H

#ifndef NEW_SERVER_PROTOCOLS_LOGIC_H
#define NEW_SERVER_PROTOCOLS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class NewServerProtocolsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, frameSettingsParentWireguardVisible)
    AUTO_PROPERTY(bool, pushButtonSettingsCloakChecked)
    AUTO_PROPERTY(bool, pushButtonSettingsSsChecked)
    AUTO_PROPERTY(bool, pushButtonSettingsOpenvpnChecked)
    AUTO_PROPERTY(QString, lineEditCloakPortText)
    AUTO_PROPERTY(QString, lineEditCloakSiteText)
    AUTO_PROPERTY(QString, lineEditSsPortText)
    AUTO_PROPERTY(QString, comboBoxSsCipherText)
    AUTO_PROPERTY(QString, lineEditOpenvpnPortText)
    AUTO_PROPERTY(QString, comboBoxOpenvpnProtoText)
    AUTO_PROPERTY(bool, checkBoxCloakChecked)
    AUTO_PROPERTY(bool, checkBoxSsChecked)
    AUTO_PROPERTY(bool, checkBoxOpenVpnChecked)
    AUTO_PROPERTY(double, progressBarConnectionMinimum)
    AUTO_PROPERTY(double, progressBarConnectionMaximum)

public:
    Q_INVOKABLE void updatePage() override;

public:
    explicit NewServerProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerProtocolsLogic() = default;

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;

signals:
    void pushButtonConfigureClicked();

};
#endif // NEW_SERVER_PROTOCOLS_LOGIC_H

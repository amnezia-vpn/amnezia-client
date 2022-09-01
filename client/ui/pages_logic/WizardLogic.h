#ifndef WIZARD_LOGIC_H
#define WIZARD_LOGIC_H

#include "PageLogicBase.h"
#include "containers/containers_defs.h"

class UiLogic;

class WizardLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, radioButtonHighChecked)
    AUTO_PROPERTY(bool, radioButtonMediumChecked)
    AUTO_PROPERTY(bool, radioButtonLowChecked)
    AUTO_PROPERTY(bool, checkBoxVpnModeChecked)
    AUTO_PROPERTY(QString, lineEditHighWebsiteMaskingText)

public:
    Q_INVOKABLE void onUpdatePage() override;
    Q_INVOKABLE void onPushButtonVpnModeFinishClicked();
    Q_INVOKABLE void onPushButtonLowFinishClicked();

public:
    explicit WizardLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~WizardLogic() = default;

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

};
#endif // WIZARD_LOGIC_H

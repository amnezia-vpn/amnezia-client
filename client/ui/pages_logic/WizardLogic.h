#ifndef WIZARD_LOGIC_H
#define WIZARD_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class WizardLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, radioButtonSetupWizardHighChecked)
    AUTO_PROPERTY(bool, radioButtonSetupWizardMediumChecked)
    AUTO_PROPERTY(bool, radioButtonSetupWizardLowChecked)
    AUTO_PROPERTY(bool, checkBoxSetupWizardVpnModeChecked)
    AUTO_PROPERTY(QString, lineEditSetupWizardHighWebsiteMaskingText)

public:
    Q_INVOKABLE void updateWizardHighPage();
    Q_INVOKABLE void onPushButtonSetupWizardVpnModeFinishClicked();
    Q_INVOKABLE void onPushButtonSetupWizardLowFinishClicked();

public:
    explicit WizardLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~WizardLogic() = default;

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

};
#endif // WIZARD_LOGIC_H

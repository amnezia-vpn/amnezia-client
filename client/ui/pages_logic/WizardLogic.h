#ifndef WIZARD_LOGIC_H
#define WIZARD_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class WizardLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateWizardHighPage();

    Q_PROPERTY(bool radioButtonSetupWizardHighChecked READ getRadioButtonSetupWizardHighChecked WRITE setRadioButtonSetupWizardHighChecked NOTIFY radioButtonSetupWizardHighCheckedChanged)
    Q_PROPERTY(bool radioButtonSetupWizardMediumChecked READ getRadioButtonSetupWizardMediumChecked WRITE setRadioButtonSetupWizardMediumChecked NOTIFY radioButtonSetupWizardMediumCheckedChanged)
    Q_PROPERTY(bool radioButtonSetupWizardLowChecked READ getRadioButtonSetupWizardLowChecked WRITE setRadioButtonSetupWizardLowChecked NOTIFY radioButtonSetupWizardLowCheckedChanged)
    Q_PROPERTY(bool checkBoxSetupWizardVpnModeChecked READ getCheckBoxSetupWizardVpnModeChecked WRITE setCheckBoxSetupWizardVpnModeChecked NOTIFY checkBoxSetupWizardVpnModeCheckedChanged)
    Q_PROPERTY(QString lineEditSetupWizardHighWebsiteMaskingText READ getLineEditSetupWizardHighWebsiteMaskingText WRITE setLineEditSetupWizardHighWebsiteMaskingText NOTIFY lineEditSetupWizardHighWebsiteMaskingTextChanged)

    Q_INVOKABLE void onPushButtonSetupWizardVpnModeFinishClicked();
    Q_INVOKABLE void onPushButtonSetupWizardLowFinishClicked();

public:
    explicit WizardLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~WizardLogic() = default;

    bool getRadioButtonSetupWizardMediumChecked() const;
    void setRadioButtonSetupWizardMediumChecked(bool radioButtonSetupWizardMediumChecked);
    QString getLineEditSetupWizardHighWebsiteMaskingText() const;
    void setLineEditSetupWizardHighWebsiteMaskingText(const QString &lineEditSetupWizardHighWebsiteMaskingText);

    bool getRadioButtonSetupWizardHighChecked() const;
    void setRadioButtonSetupWizardHighChecked(bool radioButtonSetupWizardHighChecked);
    bool getRadioButtonSetupWizardLowChecked() const;
    void setRadioButtonSetupWizardLowChecked(bool radioButtonSetupWizardLowChecked);
    bool getCheckBoxSetupWizardVpnModeChecked() const;
    void setCheckBoxSetupWizardVpnModeChecked(bool checkBoxSetupWizardVpnModeChecked);

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

signals:
    void lineEditSetupWizardHighWebsiteMaskingTextChanged();
    void radioButtonSetupWizardHighCheckedChanged();
    void radioButtonSetupWizardMediumCheckedChanged();
    void radioButtonSetupWizardLowCheckedChanged();
    void checkBoxSetupWizardVpnModeCheckedChanged();

private:


private slots:



private:
    bool m_radioButtonSetupWizardHighChecked;
    bool m_radioButtonSetupWizardMediumChecked;
    bool m_radioButtonSetupWizardLowChecked;
    QString m_lineEditSetupWizardHighWebsiteMaskingText;
    bool m_checkBoxSetupWizardVpnModeChecked;

};
#endif // WIZARD_LOGIC_H

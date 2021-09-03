#ifndef APP_SETTINGS_LOGIC_H
#define APP_SETTINGS_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class AppSettingsLogic : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateAppSettingsPage();

    Q_PROPERTY(bool checkBoxAppSettingsAutostartChecked READ getCheckBoxAppSettingsAutostartChecked WRITE setCheckBoxAppSettingsAutostartChecked NOTIFY checkBoxAppSettingsAutostartCheckedChanged)
    Q_PROPERTY(bool checkBoxAppSettingsAutoconnectChecked READ getCheckBoxAppSettingsAutoconnectChecked WRITE setCheckBoxAppSettingsAutoconnectChecked NOTIFY checkBoxAppSettingsAutoconnectCheckedChanged)
    Q_PROPERTY(bool checkBoxAppSettingsStartMinimizedChecked READ getCheckBoxAppSettingsStartMinimizedChecked WRITE setCheckBoxAppSettingsStartMinimizedChecked NOTIFY checkBoxAppSettingsStartMinimizedCheckedChanged)

    Q_PROPERTY(QString labelAppSettingsVersionText READ getLabelAppSettingsVersionText WRITE setLabelAppSettingsVersionText NOTIFY labelAppSettingsVersionTextChanged)

    Q_INVOKABLE void onCheckBoxAppSettingsAutostartToggled(bool checked);
    Q_INVOKABLE void onCheckBoxAppSettingsAutoconnectToggled(bool checked);
    Q_INVOKABLE void onCheckBoxAppSettingsStartMinimizedToggled(bool checked);

    Q_INVOKABLE void onPushButtonAppSettingsOpenLogsChecked();

public:
    explicit AppSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~AppSettingsLogic() = default;


    bool getCheckBoxAppSettingsAutostartChecked() const;
    void setCheckBoxAppSettingsAutostartChecked(bool checkBoxAppSettingsAutostartChecked);
    bool getCheckBoxAppSettingsAutoconnectChecked() const;
    void setCheckBoxAppSettingsAutoconnectChecked(bool checkBoxAppSettingsAutoconnectChecked);
    bool getCheckBoxAppSettingsStartMinimizedChecked() const;
    void setCheckBoxAppSettingsStartMinimizedChecked(bool checkBoxAppSettingsStartMinimizedChecked);

    QString getLabelAppSettingsVersionText() const;
    void setLabelAppSettingsVersionText(const QString &labelAppSettingsVersionText);

signals:
    void checkBoxAppSettingsAutostartCheckedChanged();
    void checkBoxAppSettingsAutoconnectCheckedChanged();
    void checkBoxAppSettingsStartMinimizedCheckedChanged();

    void labelAppSettingsVersionTextChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    bool m_checkBoxAppSettingsAutostartChecked;
    bool m_checkBoxAppSettingsAutoconnectChecked;
    bool m_checkBoxAppSettingsStartMinimizedChecked;

    QString m_labelAppSettingsVersionText;

};
#endif // APP_SETTINGS_LOGIC_H

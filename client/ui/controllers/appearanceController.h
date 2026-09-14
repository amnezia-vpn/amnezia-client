#ifndef APPEARANCECONTROLLER_H
#define APPEARANCECONTROLLER_H

#include <QObject>

#include "core/controllers/settingsController.h"

class AppearanceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int themeMode READ getThemeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool isDarkMode READ isDarkMode NOTIFY isDarkModeChanged)

public:
    enum class ThemeMode { System = 0, Light = 1, Dark = 2 };

    explicit AppearanceController(SettingsController *settingsController, QObject *parent = nullptr);

    int getThemeMode() const;
    void setThemeMode(int mode);
    bool isDarkMode() const;

signals:
    void themeModeChanged();
    void isDarkModeChanged();

private:
    bool resolveIsDark() const;
    void updateResolvedTheme();

    SettingsController *m_settingsController;
    ThemeMode m_themeMode;
    bool m_isDark;
};

#endif // APPEARANCECONTROLLER_H

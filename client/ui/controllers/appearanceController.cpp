#include "appearanceController.h"

#include <QGuiApplication>
#include <QStyleHints>

namespace
{
    int clampMode(int mode)
    {
        if (mode < static_cast<int>(AppearanceController::ThemeMode::System)
            || mode > static_cast<int>(AppearanceController::ThemeMode::Dark)) {
            return static_cast<int>(AppearanceController::ThemeMode::System);
        }
        return mode;
    }
}

AppearanceController::AppearanceController(SettingsController *settingsController, QObject *parent)
    : QObject(parent),
      m_settingsController(settingsController),
      m_themeMode(static_cast<ThemeMode>(clampMode(settingsController->getThemeMode()))),
      m_isDark(resolveIsDark())
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this](Qt::ColorScheme) { updateResolvedTheme(); });
}

int AppearanceController::getThemeMode() const
{
    return static_cast<int>(m_themeMode);
}

void AppearanceController::setThemeMode(int mode)
{
    const auto newMode = static_cast<ThemeMode>(clampMode(mode));
    if (newMode == m_themeMode) {
        return;
    }

    m_themeMode = newMode;
    m_settingsController->setThemeMode(static_cast<int>(newMode));
    emit themeModeChanged();

    updateResolvedTheme();
}

bool AppearanceController::isDarkMode() const
{
    return m_isDark;
}

bool AppearanceController::resolveIsDark() const
{
    switch (m_themeMode) {
    case ThemeMode::Light:
        return false;
    case ThemeMode::Dark:
        return true;
    default:
        return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
    }
}

void AppearanceController::updateResolvedTheme()
{
    const bool isDark = resolveIsDark();
    if (isDark == m_isDark) {
        return;
    }

    m_isDark = isDark;
    emit isDarkModeChanged();
}

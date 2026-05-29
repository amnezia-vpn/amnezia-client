/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxutils.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

namespace {

bool paletteSuggestsDarkTheme()
{
    const QPalette palette = QGuiApplication::palette();
    const int windowLightness = palette.color(QPalette::Window).lightness();
    const int textLightness = palette.color(QPalette::WindowText).lightness();

    if (textLightness - windowLightness > 50) {
        return true;
    }
    if (windowLightness - textLightness > 50) {
        return false;
    }

    return windowLightness < 128;
}

class LinuxThemeObserver final : public QObject {
    Q_OBJECT

public:
    explicit LinuxThemeObserver(std::function<void()> callback, QObject *parent = nullptr)
        : QObject(parent)
        , m_callback(std::move(callback))
    {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (!bus.isConnected()) {
            return;
        }

        bus.connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                    QStringLiteral("/org/freedesktop/portal/desktop"),
                    QStringLiteral("org.freedesktop.portal.Settings"),
                    QStringLiteral("SettingChanged"),
                    this,
                    SLOT(onPortalSettingChanged(QString, QString, QDBusVariant)));
    }

private slots:
    void onPortalSettingChanged(const QString &namespaceName, const QString &key, const QDBusVariant &value)
    {
        Q_UNUSED(value);

        if (namespaceName == QStringLiteral("org.freedesktop.appearance")
            && key == QStringLiteral("color-scheme") && m_callback) {
            m_callback();
        }
    }

private:
    std::function<void()> m_callback;
};

LinuxThemeObserver *g_themeObserver = nullptr;

} // namespace

bool LinuxUtils::isDarkTheme()
{
    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        switch (styleHints->colorScheme()) {
        case Qt::ColorScheme::Dark:
            return true;
        case Qt::ColorScheme::Light:
            return false;
        case Qt::ColorScheme::Unknown:
        default:
            break;
        }
    }

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("gtk-3.0"),
                       QStringLiteral("settings"));
    const QString themeName = settings.value(QStringLiteral("gtk-theme-name")).toString();
    if (themeName.contains(QStringLiteral("dark"), Qt::CaseInsensitive)) {
        return true;
    }
    if (themeName.contains(QStringLiteral("light"), Qt::CaseInsensitive)) {
        return false;
    }

    QSettings kdeSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("kdeglobals"));
    const QString colorScheme = kdeSettings.value(QStringLiteral("General/ColorScheme")).toString();
    if (colorScheme.contains(QStringLiteral("dark"), Qt::CaseInsensitive)) {
        return true;
    }
    if (colorScheme.contains(QStringLiteral("light"), Qt::CaseInsensitive)) {
        return false;
    }

    return paletteSuggestsDarkTheme();
}

void LinuxUtils::installThemeChangeObserver(std::function<void()> callback)
{
    if (!callback) {
        return;
    }

    if (!g_themeObserver) {
        g_themeObserver = new LinuxThemeObserver(std::move(callback), qApp);
    }
}

#include "linuxutils.moc"

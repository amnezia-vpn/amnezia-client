#ifndef SHORTCUTCONTROLLER_H
#define SHORTCUTCONTROLLER_H

#include <QObject>

#include "settings.h"

class ConnectionController;

#ifdef Q_OS_MACOS
class MacOSGlobalHotkeyManager;
#endif

class ShortcutController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString shortcutText READ shortcutText NOTIFY shortcutTextChanged)
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(bool supported READ isSupported CONSTANT)
    Q_PROPERTY(bool hasShortcut READ hasShortcut NOTIFY shortcutTextChanged)

public:
    explicit ShortcutController(const std::shared_ptr<Settings> &settings, ConnectionController *connectionController,
                                QObject *parent = nullptr);

    bool isEnabled() const;
    QString shortcutText() const;
    bool isRecording() const;
    bool isSupported() const;
    bool hasShortcut() const;

public slots:
    void setEnabled(bool enabled);
    void startRecording();
    void cancelRecording();
    void clearShortcut();
    void captureShortcut(int key, int modifiers, int nativeScanCode);

signals:
    void enabledChanged(bool enabled);
    void shortcutTextChanged();
    void recordingChanged(bool recording);
    void shortcutRegistrationError(const QString &message);

private:
    bool registerStoredShortcut(bool notifyOnError);
    void emitShortcutState();
    static bool isModifierOnlyKey(int key);
    static QString shortcutTextFromQt(int key, int modifiers);

#ifdef Q_OS_MACOS
    static quint32 carbonModifiersFromQt(int modifiers);
    static quint32 carbonModifiersFromShortcutText(const QString &shortcutText);
    static quint32 macKeyCodeFromQtKey(int key);
    static bool isModifierKeyCode(quint32 keyCode);
    bool normalizeStoredShortcut();

private slots:
    void onHotKeyActivated();
#endif

private:

    std::shared_ptr<Settings> m_settings;
    ConnectionController *m_connectionController = nullptr;
    bool m_recording = false;

#ifdef Q_OS_MACOS
    MacOSGlobalHotkeyManager *m_hotKeyManager = nullptr;
#endif
};

#endif // SHORTCUTCONTROLLER_H

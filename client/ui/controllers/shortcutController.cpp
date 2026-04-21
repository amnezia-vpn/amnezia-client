#include "shortcutController.h"

#include <QKeySequence>
#include <QMetaObject>
#include <QStringList>

#include "logger.h"
#include "ui/controllers/connectionController.h"

#ifdef Q_OS_MACOS
    #include "platforms/macos/macosglobalhotkeymanager.h"
    #include "platforms/macos/macosutils.h"
#endif

namespace
{
    Logger logger("ShortcutController");

#ifdef Q_OS_MACOS
    QString carbonModifiersToString(quint32 modifiers)
    {
        QStringList parts;
        if (modifiers & cmdKey) {
            parts << "cmd";
        }
        if (modifiers & shiftKey) {
            parts << "shift";
        }
        if (modifiers & optionKey) {
            parts << "option";
        }
        if (modifiers & controlKey) {
            parts << "control";
        }

        return parts.isEmpty() ? QStringLiteral("none") : parts.join(QLatin1Char('+'));
    }
#endif
}

ShortcutController::ShortcutController(const std::shared_ptr<Settings> &settings, ConnectionController *connectionController,
                                       QObject *parent)
    : QObject(parent), m_settings(settings), m_connectionController(connectionController)
{
    logger.debug() << "Created. Stored enabled:" << m_settings->isGlobalShortcutEnabled()
                   << "stored shortcut:" << m_settings->globalShortcutText()
                   << "keyCode:" << static_cast<uint64_t>(m_settings->globalShortcutKeyCode())
                   << "modifiers:" << static_cast<uint64_t>(m_settings->globalShortcutModifiers());
#ifdef Q_OS_MACOS
    normalizeStoredShortcut();
    m_hotKeyManager = new MacOSGlobalHotkeyManager(this);
    connect(m_hotKeyManager, &MacOSGlobalHotkeyManager::activated, this, &ShortcutController::onHotKeyActivated);

    if (m_settings->isGlobalShortcutEnabled() && !registerStoredShortcut(false)) {
        logger.warning() << "Stored shortcut could not be restored. Disabling it.";
        m_settings->setGlobalShortcutEnabled(false);
    }
#endif

    connect(m_settings.get(), &Settings::settingsCleared, this, [this]() {
        logger.debug() << "Settings cleared, removing shortcut state";
        clearShortcut();
        emitShortcutState();
    });
}

bool ShortcutController::isEnabled() const
{
    return isSupported() && m_settings->isGlobalShortcutEnabled();
}

QString ShortcutController::shortcutText() const
{
    return m_settings->globalShortcutText();
}

bool ShortcutController::isRecording() const
{
    return m_recording;
}

bool ShortcutController::isSupported() const
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

bool ShortcutController::hasShortcut() const
{
    return !shortcutText().isEmpty() && m_settings->globalShortcutKeyCode() > 0 && m_settings->globalShortcutModifiers() > 0;
}

void ShortcutController::setEnabled(bool enabled)
{
    logger.debug() << "Set enabled requested:" << enabled
                   << "current enabled:" << isEnabled()
                   << "hasShortcut:" << hasShortcut()
                   << "shortcut:" << shortcutText();
    if (!isSupported() || enabled == isEnabled()) {
        return;
    }

    if (enabled) {
        if (!hasShortcut()) {
            emit shortcutRegistrationError(tr("Set a keyboard shortcut first"));
            return;
        }

        if (!registerStoredShortcut(true)) {
            return;
        }
    }
#ifdef Q_OS_MACOS
    else if (m_hotKeyManager) {
        m_hotKeyManager->unregisterHotKey();
    }
#endif

    m_settings->setGlobalShortcutEnabled(enabled);
    logger.debug() << "Shortcut enabled state changed to:" << enabled;
    emit enabledChanged(enabled);
}

void ShortcutController::startRecording()
{
    if (!isSupported()) {
        emit shortcutRegistrationError(tr("Global shortcuts are only available on macOS"));
        return;
    }

    if (!m_recording) {
        m_recording = true;
        logger.debug() << "Shortcut recording started";
        emit recordingChanged(true);
    }
}

void ShortcutController::cancelRecording()
{
    if (m_recording) {
        m_recording = false;
        logger.debug() << "Shortcut recording cancelled";
        emit recordingChanged(false);
    }
}

void ShortcutController::clearShortcut()
{
    logger.debug() << "Clearing stored shortcut. Was enabled:" << isEnabled()
                   << "shortcut:" << shortcutText();
#ifdef Q_OS_MACOS
    if (m_hotKeyManager) {
        m_hotKeyManager->unregisterHotKey();
    }
#endif

    const bool wasEnabled = isEnabled();
    const bool hadShortcut = hasShortcut();

    m_settings->setGlobalShortcutEnabled(false);
    m_settings->setGlobalShortcutKeyCode(0);
    m_settings->setGlobalShortcutModifiers(0);
    m_settings->setGlobalShortcutText({});

    cancelRecording();

    if (wasEnabled) {
        emit enabledChanged(false);
    }
    if (hadShortcut) {
        emit shortcutTextChanged();
    }
}

void ShortcutController::captureShortcut(int key, int modifiers, int nativeScanCode)
{
    if (!m_recording || !isSupported()) {
        return;
    }

    if (isModifierOnlyKey(key)) {
        return;
    }

    int macKeyCode = nativeScanCode;
    bool usedNativeScanCode = macKeyCode > 0 && !isModifierKeyCode(static_cast<quint32>(macKeyCode));
    bool usedCurrentEventKeyCode = false;
    bool usedQtFallback = false;
#ifdef Q_OS_MACOS
    if (!usedNativeScanCode) {
        if (macKeyCode > 0) {
            logger.debug() << "Captured native key code looks like a modifier, ignoring it:"
                           << static_cast<uint64_t>(macKeyCode);
        }

        macKeyCode = static_cast<int>(MacOSUtils::currentEventKeyCode());
        usedCurrentEventKeyCode = macKeyCode > 0 && !isModifierKeyCode(static_cast<quint32>(macKeyCode));

        if (!usedCurrentEventKeyCode && macKeyCode > 0) {
            logger.debug() << "Current NSEvent key code looks like a modifier, ignoring it:"
                           << static_cast<uint64_t>(macKeyCode);
        }

        if (!usedCurrentEventKeyCode) {
            macKeyCode = static_cast<int>(macKeyCodeFromQtKey(key));
            usedQtFallback = macKeyCode > 0;
        }
    }
#endif

    logger.debug() << "Capture shortcut key:" << static_cast<uint64_t>(key)
                   << "modifiers:" << static_cast<uint64_t>(modifiers)
                   << "nativeScanCode:" << static_cast<uint64_t>(nativeScanCode)
                   << "usedNativeScanCode:" << usedNativeScanCode
                   << "usedCurrentEventKeyCode:" << usedCurrentEventKeyCode
                   << "usedQtFallback:" << usedQtFallback
                   << "resolvedMacKeyCode:" << static_cast<uint64_t>(macKeyCode);

    if (macKeyCode <= 0) {
        emit shortcutRegistrationError(tr("Unable to detect the pressed key"));
        return;
    }

    const quint32 carbonModifiers = carbonModifiersFromQt(modifiers);
    if (carbonModifiers == 0) {
        emit shortcutRegistrationError(tr("Use at least one modifier key in the shortcut"));
        return;
    }

    const QString shortcut = shortcutTextFromQt(key, modifiers);
    if (shortcut.isEmpty()) {
        emit shortcutRegistrationError(tr("Unable to save the selected keyboard shortcut"));
        return;
    }

    logger.debug() << "Saving shortcut:" << shortcut;

    const bool wasEnabled = isEnabled();
    const int oldKeyCode = m_settings->globalShortcutKeyCode();
    const int oldModifiers = m_settings->globalShortcutModifiers();
    const QString oldShortcutText = m_settings->globalShortcutText();

#ifdef Q_OS_MACOS
    if (m_hotKeyManager) {
        m_hotKeyManager->unregisterHotKey();
    }
#endif

    m_settings->setGlobalShortcutKeyCode(macKeyCode);
    m_settings->setGlobalShortcutModifiers(static_cast<int>(carbonModifiers));
    m_settings->setGlobalShortcutText(shortcut);
    m_settings->setGlobalShortcutEnabled(true);

    if (!registerStoredShortcut(true)) {
        logger.warning() << "Failed to register captured shortcut, restoring previous one";
        m_settings->setGlobalShortcutKeyCode(oldKeyCode);
        m_settings->setGlobalShortcutModifiers(oldModifiers);
        m_settings->setGlobalShortcutText(oldShortcutText);
        m_settings->setGlobalShortcutEnabled(wasEnabled);

        if (wasEnabled) {
            registerStoredShortcut(false);
        }
        return;
    }

    cancelRecording();
    logger.debug() << "Shortcut captured and registered successfully:" << shortcut
                   << "keyCode:" << static_cast<uint64_t>(macKeyCode)
                   << "carbonModifiers:" << static_cast<uint64_t>(carbonModifiers);
    emit shortcutTextChanged();
    if (!wasEnabled) {
        emit enabledChanged(true);
    }
}

bool ShortcutController::registerStoredShortcut(bool notifyOnError)
{
#ifdef Q_OS_MACOS
    normalizeStoredShortcut();

    if (!m_hotKeyManager || !hasShortcut()) {
        logger.warning() << "Register stored shortcut skipped. Manager exists:" << (m_hotKeyManager != nullptr)
                         << "hasShortcut:" << hasShortcut()
                         << "shortcut:" << shortcutText();
        if (notifyOnError) {
            emit shortcutRegistrationError(tr("Set a keyboard shortcut first"));
        }
        return false;
    }

    logger.debug() << "Registering stored shortcut:" << shortcutText()
                   << "keyCode:" << static_cast<uint64_t>(m_settings->globalShortcutKeyCode())
                   << "modifiers:" << static_cast<uint64_t>(m_settings->globalShortcutModifiers())
                   << "(" << carbonModifiersToString(static_cast<quint32>(m_settings->globalShortcutModifiers())) << ")";
    const bool isRegistered = m_hotKeyManager->registerHotKey(
            static_cast<quint32>(m_settings->globalShortcutKeyCode()),
            static_cast<quint32>(m_settings->globalShortcutModifiers()));

    logger.debug() << "Stored shortcut registration result:" << isRegistered;
    if (!isRegistered && notifyOnError) {
        emit shortcutRegistrationError(tr("The selected keyboard shortcut is unavailable or already in use"));
    }

    return isRegistered;
#else
    Q_UNUSED(notifyOnError);
    return false;
#endif
}

void ShortcutController::emitShortcutState()
{
    logger.debug() << "Emit shortcut state enabled:" << isEnabled()
                   << "recording:" << m_recording
                   << "shortcut:" << shortcutText();
    emit enabledChanged(isEnabled());
    emit shortcutTextChanged();
    emit recordingChanged(m_recording);
}

bool ShortcutController::isModifierOnlyKey(int key)
{
    switch (key) {
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
        return true;
    default:
        return false;
    }
}

QString ShortcutController::shortcutTextFromQt(int key, int modifiers)
{
    return QKeySequence(key | modifiers).toString(QKeySequence::NativeText);
}

#ifdef Q_OS_MACOS
quint32 ShortcutController::carbonModifiersFromQt(int modifiers)
{
    quint32 carbonModifiers = 0;

    if (modifiers & Qt::ShiftModifier) {
        carbonModifiers |= shiftKey;
    }
    if (modifiers & Qt::AltModifier) {
        carbonModifiers |= optionKey;
    }
    // On macOS Qt reports the Command key as Qt::ControlModifier and
    // the physical Control key as Qt::MetaModifier.
    if (modifiers & Qt::ControlModifier) {
        carbonModifiers |= cmdKey;
    }
    if (modifiers & Qt::MetaModifier) {
        carbonModifiers |= controlKey;
    }

    return carbonModifiers;
}

quint32 ShortcutController::carbonModifiersFromShortcutText(const QString &shortcutText)
{
    quint32 carbonModifiers = 0;

    if (shortcutText.contains(QChar(0x2318))
        || shortcutText.contains(QStringLiteral("cmd"), Qt::CaseInsensitive)
        || shortcutText.contains(QStringLiteral("command"), Qt::CaseInsensitive)) {
        carbonModifiers |= cmdKey;
    }
    if (shortcutText.contains(QChar(0x2303))
        || shortcutText.contains(QStringLiteral("ctrl"), Qt::CaseInsensitive)
        || shortcutText.contains(QStringLiteral("control"), Qt::CaseInsensitive)) {
        carbonModifiers |= controlKey;
    }
    if (shortcutText.contains(QChar(0x21E7))
        || shortcutText.contains(QStringLiteral("shift"), Qt::CaseInsensitive)) {
        carbonModifiers |= shiftKey;
    }
    if (shortcutText.contains(QChar(0x2325))
        || shortcutText.contains(QStringLiteral("alt"), Qt::CaseInsensitive)
        || shortcutText.contains(QStringLiteral("option"), Qt::CaseInsensitive)) {
        carbonModifiers |= optionKey;
    }

    return carbonModifiers;
}

quint32 ShortcutController::macKeyCodeFromQtKey(int key)
{
    switch (key) {
    case Qt::Key_A:
        return kVK_ANSI_A;
    case Qt::Key_B:
        return kVK_ANSI_B;
    case Qt::Key_C:
        return kVK_ANSI_C;
    case Qt::Key_D:
        return kVK_ANSI_D;
    case Qt::Key_E:
        return kVK_ANSI_E;
    case Qt::Key_F:
        return kVK_ANSI_F;
    case Qt::Key_G:
        return kVK_ANSI_G;
    case Qt::Key_H:
        return kVK_ANSI_H;
    case Qt::Key_I:
        return kVK_ANSI_I;
    case Qt::Key_J:
        return kVK_ANSI_J;
    case Qt::Key_K:
        return kVK_ANSI_K;
    case Qt::Key_L:
        return kVK_ANSI_L;
    case Qt::Key_M:
        return kVK_ANSI_M;
    case Qt::Key_N:
        return kVK_ANSI_N;
    case Qt::Key_O:
        return kVK_ANSI_O;
    case Qt::Key_P:
        return kVK_ANSI_P;
    case Qt::Key_Q:
        return kVK_ANSI_Q;
    case Qt::Key_R:
        return kVK_ANSI_R;
    case Qt::Key_S:
        return kVK_ANSI_S;
    case Qt::Key_T:
        return kVK_ANSI_T;
    case Qt::Key_U:
        return kVK_ANSI_U;
    case Qt::Key_V:
        return kVK_ANSI_V;
    case Qt::Key_W:
        return kVK_ANSI_W;
    case Qt::Key_X:
        return kVK_ANSI_X;
    case Qt::Key_Y:
        return kVK_ANSI_Y;
    case Qt::Key_Z:
        return kVK_ANSI_Z;
    case Qt::Key_0:
        return kVK_ANSI_0;
    case Qt::Key_1:
        return kVK_ANSI_1;
    case Qt::Key_2:
        return kVK_ANSI_2;
    case Qt::Key_3:
        return kVK_ANSI_3;
    case Qt::Key_4:
        return kVK_ANSI_4;
    case Qt::Key_5:
        return kVK_ANSI_5;
    case Qt::Key_6:
        return kVK_ANSI_6;
    case Qt::Key_7:
        return kVK_ANSI_7;
    case Qt::Key_8:
        return kVK_ANSI_8;
    case Qt::Key_9:
        return kVK_ANSI_9;
    case Qt::Key_Minus:
        return kVK_ANSI_Minus;
    case Qt::Key_Equal:
        return kVK_ANSI_Equal;
    case Qt::Key_BracketLeft:
        return kVK_ANSI_LeftBracket;
    case Qt::Key_BracketRight:
        return kVK_ANSI_RightBracket;
    case Qt::Key_Backslash:
        return kVK_ANSI_Backslash;
    case Qt::Key_Semicolon:
        return kVK_ANSI_Semicolon;
    case Qt::Key_Apostrophe:
        return kVK_ANSI_Quote;
    case Qt::Key_Comma:
        return kVK_ANSI_Comma;
    case Qt::Key_Period:
        return kVK_ANSI_Period;
    case Qt::Key_Slash:
        return kVK_ANSI_Slash;
    case Qt::Key_QuoteLeft:
        return kVK_ANSI_Grave;
    case Qt::Key_Space:
        return kVK_Space;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return kVK_Tab;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return kVK_Return;
    case Qt::Key_Escape:
        return kVK_Escape;
    case Qt::Key_Backspace:
        return kVK_Delete;
    case Qt::Key_Delete:
        return kVK_ForwardDelete;
    case Qt::Key_Home:
        return kVK_Home;
    case Qt::Key_End:
        return kVK_End;
    case Qt::Key_PageUp:
        return kVK_PageUp;
    case Qt::Key_PageDown:
        return kVK_PageDown;
    case Qt::Key_Left:
        return kVK_LeftArrow;
    case Qt::Key_Right:
        return kVK_RightArrow;
    case Qt::Key_Up:
        return kVK_UpArrow;
    case Qt::Key_Down:
        return kVK_DownArrow;
    case Qt::Key_F1:
        return kVK_F1;
    case Qt::Key_F2:
        return kVK_F2;
    case Qt::Key_F3:
        return kVK_F3;
    case Qt::Key_F4:
        return kVK_F4;
    case Qt::Key_F5:
        return kVK_F5;
    case Qt::Key_F6:
        return kVK_F6;
    case Qt::Key_F7:
        return kVK_F7;
    case Qt::Key_F8:
        return kVK_F8;
    case Qt::Key_F9:
        return kVK_F9;
    case Qt::Key_F10:
        return kVK_F10;
    case Qt::Key_F11:
        return kVK_F11;
    case Qt::Key_F12:
        return kVK_F12;
    default:
        logger.warning() << "No macOS key code mapping for Qt key:" << static_cast<uint64_t>(key);
        return 0;
    }
}

bool ShortcutController::isModifierKeyCode(quint32 keyCode)
{
    switch (keyCode) {
    case kVK_Command:
    case kVK_RightCommand:
    case kVK_Shift:
    case kVK_RightShift:
    case kVK_Option:
    case kVK_RightOption:
    case kVK_Control:
    case kVK_RightControl:
    case kVK_CapsLock:
        return true;
    default:
        return false;
    }
}

bool ShortcutController::normalizeStoredShortcut()
{
    const QString storedShortcut = shortcutText();
    if (storedShortcut.isEmpty() || m_settings->globalShortcutKeyCode() <= 0) {
        return false;
    }

    const quint32 storedModifiers = static_cast<quint32>(m_settings->globalShortcutModifiers());
    const quint32 normalizedModifiers = carbonModifiersFromShortcutText(storedShortcut);
    if (normalizedModifiers == 0) {
        logger.warning() << "Could not infer Carbon modifiers from stored shortcut text:" << storedShortcut
                         << "stored modifiers:" << static_cast<uint64_t>(storedModifiers);
        return false;
    }

    if (storedModifiers == normalizedModifiers) {
        return false;
    }

    logger.warning() << "Normalizing stored shortcut modifiers from:"
                     << static_cast<uint64_t>(storedModifiers)
                     << "(" << carbonModifiersToString(storedModifiers) << ")"
                     << "to:" << static_cast<uint64_t>(normalizedModifiers)
                     << "(" << carbonModifiersToString(normalizedModifiers) << ")"
                     << "shortcut:" << storedShortcut;
    m_settings->setGlobalShortcutModifiers(static_cast<int>(normalizedModifiers));
    return true;
}

void ShortcutController::onHotKeyActivated()
{
    logger.debug() << "Hotkey activation received. Enabled:" << isEnabled()
                   << "shortcut:" << shortcutText()
                   << "controller exists:" << (m_connectionController != nullptr);
    if (!m_connectionController) {
        return;
    }

    logger.debug() << "Invoking toggleConnectionByShortcut directly";
    m_connectionController->toggleConnectionByShortcut();
}
#endif

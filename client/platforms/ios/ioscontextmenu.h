#ifndef IOSCONTEXTMENU_H
#define IOSCONTEXTMENU_H

#include <QObject>
#include <QQuickItem>

// Presents the native iOS edit menu (UIEditMenuInteraction) for a text
// control. The menu items (Cut/Copy/Paste/Select All) are provided by the
// system based on the first responder, which is Qt's text input responder
// for the focused control.
//
// Needed because the ContextMenu attached type is backed by a native menu
// on iOS only since Qt 6.10 — on Qt 6.9 it opens a Qt-drawn menu instead.
class IosContextMenu : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE bool isAvailable() const;
    Q_INVOKABLE void present(QQuickItem *target, qreal x, qreal y);
};

#endif // IOSCONTEXTMENU_H

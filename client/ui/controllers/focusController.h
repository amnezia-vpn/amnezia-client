#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include "ui/controllers/listViewFocusController.h"

#include <QQmlApplicationEngine>

/*!
 * \brief The FocusController class makes focus control more straightforward
 * \details Focus is handled only for visible and enabled items which have
 *          `isFocused` property from top left to bottom right.
 * \note There are items handled differently (e.g. ListView)
 */
class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~FocusController() override = default;

    Q_INVOKABLE void nextKeyTabItem();
    Q_INVOKABLE void previousKeyTabItem();
    Q_INVOKABLE void nextKeyUpItem();
    Q_INVOKABLE void nextKeyDownItem();
    Q_INVOKABLE void nextKeyLeftItem();
    Q_INVOKABLE void nextKeyRightItem();
    Q_INVOKABLE void setFocusItem(QQuickItem *item);
    Q_INVOKABLE void setFocusOnDefaultItem();
    Q_INVOKABLE void pushRootObject(QObject *object);
    Q_INVOKABLE void dropRootObject(QObject *object);
    Q_INVOKABLE void resetRootObject();

private:
    enum class Direction {
        Forward,
        Backward,
    };

    void reload(Direction direction);
    void nextItem(Direction direction);
    void focusNextListViewItem();
    void focusPreviousListViewItem();
    void dropListView();

    QQmlApplicationEngine *m_engine; // Pointer to engine to get root object
    QList<QObject *> m_focusChain;   // List of current objects to be focused
    QQuickItem *m_focusedItem;       // Pointer to the active focus item
    QStack<QObject *> m_rootObjects; // Pointer to stack of roots for focus chain
    QQuickItem *m_defaultFocusItem;

    ListViewFocusController *m_lvfc; // ListView focus manager

signals:
    void focusedItemChanged();
};

#endif // FOCUSCONTROLLER_H

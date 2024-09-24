#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QObject>
#include <QStack>
#include <QSharedPointer>


class QQuickItem;
class QQmlApplicationEngine;
class ListViewFocusController;

class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine* engine, QObject *parent = nullptr);
    ~FocusController() override = default;

    Q_INVOKABLE void nextKeyTabItem();
    Q_INVOKABLE void previousKeyTabItem();
    Q_INVOKABLE void nextKeyUpItem();
    Q_INVOKABLE void nextKeyDownItem();
    Q_INVOKABLE void nextKeyLeftItem();
    Q_INVOKABLE void nextKeyRightItem();
    Q_INVOKABLE void setFocusOnDefaultItem();
    Q_INVOKABLE void resetRootObject();
    Q_INVOKABLE void pushRootObject(QObject* object);
    Q_INVOKABLE void dropRootObject(QObject* object);

private:
    void nextItem(bool isForwardOrder);
    void focusNextListViewItem();
    void focusPreviousListViewItem();
    void reload(bool isForwardOrder);

    QSharedPointer<QQmlApplicationEngine> m_engine; // Pointer to engine to get root object
    QList<QObject*> m_focusChain; // List of current objects to be focused
    QQuickItem* m_focusedItem; // Pointer to the active focus item
    qsizetype m_focusedItemIndex; // Active focus item's index in focus chain
    QStack<QObject*> m_rootObjects;
    QSharedPointer<QQuickItem> m_defaultFocusItem;

    ListViewFocusController* m_lvfc; // ListView focus manager
};

#endif // FOCUSCONTROLLER_H

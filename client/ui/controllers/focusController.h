#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QObject>

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

signals:
    void nextTabItemChanged(QObject* item);
    void previousTabItemChanged(QObject* item);
    void nextKeyUpItemChanged(QObject* item);
    void nextKeyDownItemChanged(QObject* item);
    void nextKeyLeftItemChanged(QObject* item);
    void nextKeyRightItemChanged(QObject* item);
    void focusChainChanged();
    void rootItemChanged();

public slots:
    void resetFocus();
    void reload();
    void setRootItem(QQuickItem* item);

private:
    void focusNextListViewItem();
    void focusPreviousListViewItem();

    QQmlApplicationEngine* m_engine; // Pointer to engine to get root object
    QList<QObject*> m_focusChain; // List of current objects to be focused
    QQuickItem* m_focusedItem; // Pointer to the active focus item
    qsizetype m_focusedItemIndex; // Active focus item's index in focus chain
    QQuickItem* m_rootItem;

    ListViewFocusController* m_lvfc; // ListView focus manager
};

#endif // FOCUSCONTROLLER_H
